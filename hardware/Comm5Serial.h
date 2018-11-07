#pragma once

#include <iosfwd>
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

class Comm5Serial : public CDomoticzHardwareBase, AsyncSerial
{
	enum RequestState {
		Idle,
		QueryRelayState,
		QuerySensorState
	} reqState;
	enum {
		STSTART_OCTET1,
		STSTART_OCTET2,
		STFRAME_SIZE,
		STFRAME_CONTROL,
		STFRAME_DATA,
		STFRAME_CRC1,
		STFRAME_CRC2
	} currentState;
public:
	Comm5Serial(const int ID, const std::string& devname, unsigned int baudRate = 115200);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool Connect();
	void requestDigitalInputResponseHandler(const std::string& frame);
	void requestDigitalOutputResponseHandler(const std::string& frame);
	void enableNotificationResponseHandler(const std::string& frame);
	void readCallBack(const char* data, size_t len);
protected:
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);

	void Do_Work();
	void ParseData(const unsigned char *data, const size_t len);
	void parseFrame(std::string& frame);
	bool writeFrame(const std::string&);


	void queryRelayState();
	void querySensorState();
	void enableNotifications();
private:
	std::string m_szSerialPort;
	const unsigned int m_baudRate;

	std::string frame;
	size_t frameSize;
	uint16_t frameCRC;

	bool initSensorData;
	int lastKnownSensorState;
	uint8_t relayStatus;

	bool notificationEnabled;

	bool m_bReceiverStarted;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

