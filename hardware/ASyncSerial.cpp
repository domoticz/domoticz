/*
 * File:   AsyncSerial.cpp
 * Author: Terraneo Federico
 * Distributed under the Boost Software License, Version 1.0.
 * Created on September 7, 2009, 10:46 AM
 *
 * v1.02: Fixed a bug in BufferedAsyncSerial: Using the default constructor
 * the callback was not set up and reading didn't work.
 *
 * v1.01: Fixed a bug that did not allow to reopen a closed serial port.
 *
 * v1.00: First release.
 *
 * IMPORTANT:
 * On Mac OS X boost asio's serial ports have bugs, and the usual implementation
 * of this class does not work. So a workaround class was written temporarily,
 * until asio (hopefully) will fix Mac compatibility for serial ports.
 *
 * Please note that unlike said in the documentation on OS X until asio will
 * be fixed serial port *writes* are *not* asynchronous, but at least
 * asynchronous *read* works.
 * In addition the serial port open ignores the following options: parity,
 * character size, flow, stop bits, and defaults to 8N1 format.
 * I know it is bad but at least it's better than nothing.
 *
 */
#include "stdafx.h"
#include "ASyncSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr/shared_array.hpp>  // for shared_array
#include <boost/system/error_code.hpp>       // for error_code
#include <boost/system/system_error.hpp>     // for system_error

#define BUFFER_SIZE 2048

//
//Class AsyncSerial
//

class AsyncSerialImpl
	: private domoticz::noncopyable
{
public:
  AsyncSerialImpl()
	  : io()
	  , port(io)
  {
  }

    boost::asio::io_service io; ///< Io service object
    boost::asio::serial_port port; ///< Serial port object
    boost::thread backgroundThread; ///< Thread that runs read/write operations
    bool open{ false };		    ///< True if port open
    bool error{ false };	    ///< Error flag
    mutable std::mutex errorMutex; ///< Mutex for access to error

    /// Data are queued here before they go in writeBuffer
    std::vector<char> writeQueue;
    boost::shared_array<char> writeBuffer; ///< Data being written
    size_t writeBufferSize{ 0 };	   ///< Size of writeBuffer
    std::mutex writeQueueMutex; ///< Mutex for access to writeQueue
    char readBuffer[BUFFER_SIZE]; ///< data being read

    /// Read complete callback
    boost::function<void (const char*, size_t)> callback;
};

AsyncSerial::AsyncSerial(): pimpl(new AsyncSerialImpl)
{

}

AsyncSerial::AsyncSerial(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
        : pimpl(new AsyncSerialImpl)
{
    open(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop);
}

AsyncSerial::~AsyncSerial()
{
	terminate();
}

void AsyncSerial::open(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
{
	if (isOpen())
		close();

	setErrorStatus(true); // If an exception is thrown, error_ remains true
	pimpl->port.open(devname);
	try {
		pimpl->port.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
		pimpl->port.set_option(opt_parity);
		pimpl->port.set_option(opt_csize);
		pimpl->port.set_option(opt_flow);
		pimpl->port.set_option(opt_stop);
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "ASyncSerial: Error setting options!");
		pimpl->port.close();
		throw;
	}

	pimpl->io.reset();

	// This gives some work to the io_service before it is started
	pimpl->io.post([this] { return doRead(); });

	boost::thread t([p = &pimpl->io] { p->run(); });
	pimpl->backgroundThread.swap(t);
	setErrorStatus(false); // If we get here, no error
	pimpl->open = true;    // Port is now open
}

void AsyncSerial::openOnlyBaud(const std::string& devname, unsigned int baud_rate,
	boost::asio::serial_port_base::parity /*opt_parity*/,
	boost::asio::serial_port_base::character_size /*opt_csize*/,
	boost::asio::serial_port_base::flow_control /*opt_flow*/,
	boost::asio::serial_port_base::stop_bits /*opt_stop*/)
{
	if(isOpen()) close();

	setErrorStatus(true);//If an exception is thrown, error_ remains true
	pimpl->port.open(devname);

	try {
		pimpl->port.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "ASyncSerial: Error setting options!");
		pimpl->port.close();
		throw;
	}

	pimpl->io.reset();

	//This gives some work to the io_service before it is started
	pimpl->io.post([this] { return doRead(); });

	boost::thread t([p = &pimpl->io] { p->run(); });
	pimpl->backgroundThread.swap(t);
	setErrorStatus(false);//If we get here, no error
	pimpl->open=true; //Port is now open
}

bool AsyncSerial::isOpen() const
{
    return pimpl->open;
}

bool AsyncSerial::errorStatus() const
{
    std::lock_guard<std::mutex> l(pimpl->errorMutex);
    return pimpl->error;
}

void AsyncSerial::close()
{
    if(!isOpen()) return;

    pimpl->open = false;
    pimpl->io.post([this] { doClose(); });
    pimpl->backgroundThread.join();
    pimpl->io.reset();
    if(errorStatus())
    {
        throw(boost::system::system_error(boost::system::error_code(),
                "Error while closing the device"));
    }
}

void AsyncSerial::write(const char *data, size_t size)
{
    {
        std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),data,data+size);
    }
    pimpl->io.post([this] { doWrite(); });
}

void AsyncSerial::write(const std::string &data)
{
	{
		std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
		pimpl->writeQueue.insert(pimpl->writeQueue.end(), data.c_str(), data.c_str()+data.size());
	}
	pimpl->io.post([this] { doWrite(); });
}

void AsyncSerial::write(const std::vector<char>& data)
{
    {
        std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),data.begin(),
                data.end());
    }
    pimpl->io.post([this] { doWrite(); });
}

void AsyncSerial::writeString(const std::string& s)
{
    {
        std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),s.begin(),s.end());
    }
    pimpl->io.post([this] { doWrite(); });
}

void AsyncSerial::doRead()
{
	if(isOpen()==false) return;
	pimpl->port.async_read_some(boost::asio::buffer(pimpl->readBuffer, sizeof(pimpl->readBuffer)), [this](auto &&err, auto bytes) { readEnd(err, bytes); });
}

void AsyncSerial::readEnd(const boost::system::error_code& error,
        size_t bytes_transferred)
{
    if(error)
    {
        //error can be true even because the serial port was closed.
        //In this case it is not a real error, so ignore
        if(isOpen())
        {
			_log.Log(LOG_ERROR,"Serial Port closed!... Error: %s", error.message().c_str());
        }
        terminate();
    } else {
        if(pimpl->callback) pimpl->callback(pimpl->readBuffer,
                bytes_transferred);
        doRead();
    }
}

void AsyncSerial::doWrite()
{
    //If a write operation is already in progress, do nothing
    if (pimpl->writeBuffer == nullptr)
    {
	    std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
	    pimpl->writeBufferSize = pimpl->writeQueue.size();
	    pimpl->writeBuffer.reset(new char[pimpl->writeQueue.size()]);

	    copy(pimpl->writeQueue.begin(), pimpl->writeQueue.end(), pimpl->writeBuffer.get());
	    pimpl->writeQueue.clear();
	    async_write(pimpl->port, boost::asio::buffer(pimpl->writeBuffer.get(), pimpl->writeBufferSize), [this](auto &&err, auto) { writeEnd(err); });
    }
}

void AsyncSerial::writeEnd(const boost::system::error_code& error)
{
    if(!error)
    {
        std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
        if(pimpl->writeQueue.empty())
        {
            pimpl->writeBuffer.reset();
            pimpl->writeBufferSize=0;
            sleep_milliseconds(75);
            return;
        }
        pimpl->writeBufferSize=pimpl->writeQueue.size();
        pimpl->writeBuffer.reset(new char[pimpl->writeQueue.size()]);
        copy(pimpl->writeQueue.begin(),pimpl->writeQueue.end(),
                pimpl->writeBuffer.get());
        pimpl->writeQueue.clear();
	async_write(pimpl->port, boost::asio::buffer(pimpl->writeBuffer.get(), pimpl->writeBufferSize), [this](auto &&err, auto) { writeEnd(err); });
    } else {
		try
		{
			setErrorStatus(true);
			doClose();
		}
		catch (...)
		{

		}
    }
}

void AsyncSerial::doClose()
{
    boost::system::error_code ec;
    pimpl->port.cancel(ec);
    if(ec) setErrorStatus(true);
    pimpl->port.close(ec);
    if(ec) setErrorStatus(true);
}

void AsyncSerial::setErrorStatus(bool e)
{
    std::lock_guard<std::mutex> l(pimpl->errorMutex);
    pimpl->error=e;
}

void AsyncSerial::setReadCallback(const boost::function<void (const char*, size_t)>& callback)
{
    pimpl->callback=callback;
}

/**
 * Unregister the read callback.
 */
void AsyncSerial::clearReadCallback()
{
    pimpl->callback.clear();
}

/**
 * Process a clean close by unregistering the read callback and closing the port.
 * Once this method has been called, you have to open the port and register the read callback again.
 */
void AsyncSerial::terminate(bool silent/*=true*/) {
	if (isOpen()) {
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...) {
			if (silent == false) {
				throw;
			}
		}
	}
}

