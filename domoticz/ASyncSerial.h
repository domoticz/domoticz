#ifndef ASYNCSERIAL_H
#define	ASYNCSERIAL_H

#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>

/**
 * Used internally (pimpl)
 */
class AsyncSerialImpl;

/**
 * Asynchronous serial class.
 * Intended to be a base class.
 */
class AsyncSerial: private boost::noncopyable
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
    * Write a string asynchronously. Returns immediately.
    * Can be used to send ASCII data to the serial device.
    * To send binary data, use write()
    * \param s string to send
    */
    void writeString(const std::string& s);

    ~AsyncSerial();

    /**
     * Read buffer maximum size
     */
    static const int readBufferSize=512;
private:

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

    /**
     * Callback to close serial port
     */
    void doClose();

    boost::shared_ptr<AsyncSerialImpl> pimpl;

protected:

    /**
     * To allow derived classes to report errors
     * \param e error status
     */
    void setErrorStatus(bool e);

    /**
     * To allow derived classes to set a read callback
     */
    void setReadCallback(const
            boost::function<void (const char*, size_t)>& callback);

    /**
     * To unregister the read callback in the derived class destructor so it
     * does not get called after the derived class destructor but before the
     * base class destructor
     */
    void clearReadCallback();

};

/**
 * Asynchronous serial class with read callback. User code can write data
 * from one thread, and read data will be reported through a callback called
 * from a separate thred.
 */
class CallbackAsyncSerial: public AsyncSerial
{
public:
    CallbackAsyncSerial();

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
    CallbackAsyncSerial(const std::string& devname, unsigned int baud_rate,
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
     * Set the read callback, the callback will be called from a thread
     * owned by the CallbackAsyncSerial class when data arrives from the
     * serial port.
     * \param callback the receive callback
     */
    void setCallback(const
            boost::function<void (const char*, size_t)>& callback);

    /**
     * Removes the callback. Any data received after this function call will
     * be lost.
     */
    void clearCallback();

    ~CallbackAsyncSerial();
};

#endif //ASYNCSERIAL_H
