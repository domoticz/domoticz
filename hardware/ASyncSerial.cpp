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
#include "../main/Noncopyable.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
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
    AsyncSerialImpl(): io(), port(io), backgroundThread(), open(false),
		error(false), writeBufferSize(0) {}

    boost::asio::io_service io; ///< Io service object
    boost::asio::serial_port port; ///< Serial port object
    boost::thread backgroundThread; ///< Thread that runs read/write operations
    bool open; ///< True if port open
    bool error; ///< Error flag
    mutable std::mutex errorMutex; ///< Mutex for access to error

    /// Data are queued here before they go in writeBuffer
    std::vector<char> writeQueue;
    boost::shared_array<char> writeBuffer; ///< Data being written
    size_t writeBufferSize; ///< Size of writeBuffer
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
    if(isOpen()) close();

    setErrorStatus(true);//If an exception is thrown, error_ remains true
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

    //This gives some work to the io_service before it is started
    pimpl->io.post(boost::bind(&AsyncSerial::doRead, this));

    boost::thread t(boost::bind(&boost::asio::io_service::run, &pimpl->io));
    pimpl->backgroundThread.swap(t);
    setErrorStatus(false);//If we get here, no error
    pimpl->open=true; //Port is now open
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
	pimpl->io.post(boost::bind(&AsyncSerial::doRead, this));

	boost::thread t(boost::bind(&boost::asio::io_service::run, &pimpl->io));
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
    pimpl->io.post(boost::bind(&AsyncSerial::doClose, this));
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
    pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::write(const std::string &data)
{
	{
		std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
		pimpl->writeQueue.insert(pimpl->writeQueue.end(), data.c_str(), data.c_str()+data.size());
	}
	pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::write(const std::vector<char>& data)
{
    {
        std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),data.begin(),
                data.end());
    }
    pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::writeString(const std::string& s)
{
    {
        std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),s.begin(),s.end());
    }
    pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::doRead()
{
	if(isOpen()==false) return;
    pimpl->port.async_read_some(boost::asio::buffer(pimpl->readBuffer,sizeof(pimpl->readBuffer)),
            boost::bind(&AsyncSerial::readEnd,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
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
    if(pimpl->writeBuffer==0)
    {
        std::lock_guard<std::mutex> l(pimpl->writeQueueMutex);
        pimpl->writeBufferSize=pimpl->writeQueue.size();
        pimpl->writeBuffer.reset(new char[pimpl->writeQueue.size()]);

        copy(pimpl->writeQueue.begin(),pimpl->writeQueue.end(),
                pimpl->writeBuffer.get());
        pimpl->writeQueue.clear();
        async_write(pimpl->port,boost::asio::buffer(pimpl->writeBuffer.get(),
                pimpl->writeBufferSize),
                boost::bind(&AsyncSerial::writeEnd, this, boost::asio::placeholders::error));
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
        async_write(pimpl->port,boost::asio::buffer(pimpl->writeBuffer.get(),
                pimpl->writeBufferSize),
                boost::bind(&AsyncSerial::writeEnd, this, boost::asio::placeholders::error));
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

