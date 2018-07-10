#pragma once
#ifndef ASYNCSERIAL_H
#define	ASYNCSERIAL_H

#include <stddef.h>                              // for size_t
#include "../main/Noncopyable.h"
#include <boost/asio/serial_port_base.hpp>       // for serial_port_base
#include <boost/smart_ptr/shared_ptr.hpp>        // for shared_ptr
#include <boost/function/function_fwd.hpp>       // for function
#include <boost/exception/diagnostic_information.hpp> //for exception printing
namespace boost { namespace system { class error_code; } }

/**
 * Used internally (pimpl)
 */
class AsyncSerialImpl;

/**
 * Asynchronous serial class.
 * Intended to be a base class.
 */
class AsyncSerial
	: private domoticz::noncopyable
{
public:
    AsyncSerial();

    /**
     * Constructor. Creates and opens a serial device.
     * \param devname serial device name, example "/dev/ttyS0" or "COM1"
     * \param baud_rate serial baud rate
     * \param opt_parity serial parity, default none
     * \param opt_csize serial character size, default 8bit
     * \param opt_flow serial flow control, default none
     * \param opt_stop serial stop bits, default 1
     * \throws boost::system::system_error if cannot open the
     * serial device
     */
    AsyncSerial(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    /**
    * Opens a serial device.
    * \param devname serial device name, example "/dev/ttyS0" or "COM1"
    * \param baud_rate serial baud rate
    * \param opt_parity serial parity, default none
    * \param opt_csize serial character size, default 8bit
    * \param opt_flow serial flow control, default none
    * \param opt_stop serial stop bits, default 1
    * \throws boost::system::system_error if cannot open the
    * serial device
    */
    void open(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));
	void openOnlyBaud(const std::string& devname, unsigned int baud_rate,
		boost::asio::serial_port_base::parity opt_parity=
		boost::asio::serial_port_base::parity(
		boost::asio::serial_port_base::parity::none),
		boost::asio::serial_port_base::character_size opt_csize=
		boost::asio::serial_port_base::character_size(8),
		boost::asio::serial_port_base::flow_control opt_flow=
		boost::asio::serial_port_base::flow_control(
		boost::asio::serial_port_base::flow_control::none),
		boost::asio::serial_port_base::stop_bits opt_stop=
		boost::asio::serial_port_base::stop_bits(
		boost::asio::serial_port_base::stop_bits::one));

    /**
     * \return true if serial device is open
     */
    bool isOpen() const;

    /**
     * \return true if error were found
     */
    bool errorStatus() const;

    /**
     * Close the serial device
     * \throws boost::system::system_error if any error
     */
    void close();

    /**
     * Write data asynchronously. Returns immediately.
     * \param data array of char to be sent through the serial device
     * \param size array size
     */
    void write(const char *data, size_t size);

     /**
     * Write data asynchronously. Returns immediately.
     * \param data to be sent through the serial device
     */
    void write(const std::vector<char>& data);

	/**
	* Write data asynchronously. Returns immediately.
	* \param string to be sent through the serial device
	*/
	void write(const std::string &data);

    /**
    * Write a string asynchronously. Returns immediately.
    * Can be used to send ASCII data to the serial device.
    * To send binary data, use write()
    * \param s string to send
    */
    void writeString(const std::string& s);

	/**
	 * Destructor. If necessary it silently removes the read callback and close the serial port. 
	 */
	~AsyncSerial();

private:
    /**
     * Callback to close serial port
     */
    void doClose();

    /**
     * Callback called to start an asynchronous read operation.
     * This callback is called by the io_service in the spawned thread.
     */
    void doRead();

    /**
     * Callback called at the end of the asynchronous operation.
     * This callback is called by the io_service in the spawned thread.
     */
    void readEnd(const boost::system::error_code& error,
        size_t bytes_transferred);

    /**
     * Callback called to start an asynchronous write operation.
     * If it is already in progress, does nothing.
     * This callback is called by the io_service in the spawned thread.
     */
    void doWrite();

    /**
     * Callback called at the end of an asynchronuous write operation,
     * if there is more data to write, restarts a new write operation.
     * This callback is called by the io_service in the spawned thread.
     */
    void writeEnd(const boost::system::error_code& error);

	std::shared_ptr<AsyncSerialImpl> pimpl;

    /**
     * To allow derived classes to report errors
     * \param e error status
     */
    void setErrorStatus(bool e);

protected:

    /**
     * To allow derived classes to set a read callback
     */
    void setReadCallback(const
            boost::function<void (const char*, size_t)>& callback);

    /**
     * Unregister the read callback.
     */
    void clearReadCallback();

    /**
     * Process a clean close by unregistering the read callback and closing the port.
     * Once this method has been called, you have to open the port and register the read callback again.
     */
    void terminate(bool silent = true);

};


#endif //ASYNCSERIAL_H
