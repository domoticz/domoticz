#pragma once
#ifndef BUFFEREDP1ASYNCSERIAL_H
#define BUFFEREDP1ASYNCSERIAL_H

#include "ASyncSerial.h"
#include "P1MeterBase.h"

class P1MeterSerial : public AsyncSerial, public P1MeterBase
{
      public:
	P1MeterSerial(int ID, const std::string &devname, unsigned int baud_rate, bool disable_crc, int ratelimit, const std::string &DecryptionKey);
	P1MeterSerial(const std::string &devname, unsigned int baud_rate,
		      boost::asio::serial_port_base::parity opt_parity = boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even),
		      boost::asio::serial_port_base::character_size opt_csize = boost::asio::serial_port_base::character_size(7),
		      boost::asio::serial_port_base::flow_control opt_flow = boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none),
		      boost::asio::serial_port_base::stop_bits opt_stop = boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
	~P1MeterSerial() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void readCallback(const char *data, size_t len);

      private:
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	std::shared_ptr<std::thread> m_thread;
};

#endif // BUFFEREDP1ASYNCSERIAL_H
