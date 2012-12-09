#pragma once
#ifndef BUFFEREDASYNCSERIAL_H
#define	BUFFEREDASYNCSERIAL_H

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define P1_SMID "/ISk5\\" // Smart Meter ID. Used to detect start of telegram.
#define P1PU1 "1-0:1.8.1" // total power usage normal tariff
#define P1PU2 "1-0:1.8.2" // total power usage low tariff
#define P1PD1 "1-0:2.8.1" // total delivered power normal tariff
#define P1PD2 "1-0:2.8.2" // total delivered power low tariff
#define P1TIP "0-0:96.14.0" // tariff indicator power
#define P1PUC "1-0:1.7.0" // current power usage
#define P1PDC "1-0:2.7.0" // current power delivery
#define P1GTS "0-1:24.3.0" // timestamp gas usage sample



class P1MeterSerial: public AsyncSerial, public CDomoticzHardwareBase
{
public:
	P1MeterSerial(const int ID, const std::string& devname, unsigned int baud_rate);
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
    P1MeterSerial(const std::string& devname, unsigned int baud_rate,
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

    ~P1MeterSerial();
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;

	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	unsigned char m_linecount;
	unsigned char m_exclmarkfound;
	void MatchLine();
	bool StartHardware();
	bool StopHardware();
    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
};

#endif //BUFFEREDASYNCSERIAL_H
