#pragma once
#ifndef BUFFEREDP1ASYNCSERIAL_H
#define	BUFFEREDP1ASYNCSERIAL_H

#include "ASyncSerial.h"
#include "P1MeterBase.h"

class P1MeterSerial: public AsyncSerial, public P1MeterBase
{
public:
	P1MeterSerial(const int ID, const std::string& devname, unsigned int baud_rate);
    /**
    * Opens a serial device.
    * \param devname serial device name, example "/dev/ttyS0" or "COM1"
    * \param baud_rate serial baud rate
    * \param opt_parity serial parity, default even
    * \param opt_csize serial character size, default 7bit
    * \param opt_flow serial flow control, default none
    * \param opt_stop serial stop bits, default 1
    * \throws boost::system::system_error if cannot open the
    * serial device
    */
    P1MeterSerial(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::even),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(7),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    ~P1MeterSerial();
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;

	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
};

#endif //BUFFEREDP1ASYNCSERIAL_H
