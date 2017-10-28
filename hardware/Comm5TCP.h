#pragma once

#include <iosfwd>
#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class Comm5TCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	Comm5TCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);

	bool WriteToHardware(const char *pdata, const unsigned char length);

public: // signals
	boost::signals2::signal<void()>	sDisconnected;

private:
	bool StartHardware();
	bool StopHardware();

	bool Connect();

protected:
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	void Do_Work();
	void ParseData(const unsigned char *data, const size_t len);


	void queryRelayState();
	void querySensorState();
	void enableNotifications();

	void processSensorData(const std::string& line);

private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	std::string buffer;
	bool initSensorData;
	unsigned int lastKnownSensorState;

	bool notificationEnabled;

	bool m_bReceiverStarted;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

