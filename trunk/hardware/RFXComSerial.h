#pragma once
#ifndef BUFFEREDASYNCSERIAL_H
#define	BUFFEREDASYNCSERIAL_H

#include "ASyncSerial.h"
#include "DomoticzHardware.h"
#include "serial/serial.h"
#include <map>

class RFXComSerial: public AsyncSerial, public CDomoticzHardwareBase
{
public:
	RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate);
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
    RFXComSerial(const std::string& devname, unsigned int baud_rate,
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

    ~RFXComSerial();
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;

	bool m_bReceiverStarted;

	bool WriteToHardware(const char *pdata, const unsigned char length);
	bool UploadFirmware(const std::string &szFilename);
	float GetUploadPercentage(); //returns -1 when failed
	std::string GetUploadMessage();
private:
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice(const bool bIsFirmwareUpgrade=false);
	void Do_Work();
	bool onInternalMessage(const unsigned char *pBuffer, const size_t Len);

	bool UpgradeFirmware();
	bool Write_TX_PKT(const unsigned char *pdata, size_t length, const int max_retry = 3);
	bool Handle_RX_PKT(const unsigned char *pdata, size_t length);
	bool Read_Firmware_File(const char *szFilename);
	bool EraseMemory(const int StartAddress, const int StopAddress);

	serial::Serial m_serial;
	bool m_bStartFirmwareUpload;
	std::string m_szFirmwareFile;
	std::string m_szUploadMessage;
	float m_FirmwareUploadPercentage;
	bool m_bInBootloaderMode;
	bool m_bHaveRX;
	std::map<unsigned long, std::string> m_Firmware_Buffer;

	unsigned char m_rx_input_buffer[512];
	int m_rx_tot_bytes;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;

    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
};

#endif //BUFFEREDASYNCSERIAL_H
