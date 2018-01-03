#pragma once
#include "BasePush.h"

namespace http {
	namespace server {
		class CWebsocketHandler;
	}
}

class CWebSocketPush : public CBasePush
{
public:
	explicit CWebSocketPush(http::server::CWebsocketHandler *sock);
	void Start();
	void Stop();
	void ListenTo(const unsigned long long DeviceRowIdx);
	void UnlistenTo(const unsigned long long DeviceRowIdx);
	void ClearListenTable();
	void ListenToRoomplan();
	void UnlistenToRoomplan();
	void ListenToDeviceTable();
	void UnlistenToDeviceTable();
	void onRoomplanChanged();
	void onDeviceTableChanged(); // device added, or deleted
	// etc, we need a notification of all changes that need to be reflected in the UI
	bool WeListenTo(const unsigned long long DeviceRowIdx);
private:
	void OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void OnNotificationReceived(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification);
	bool listenRoomplan;
	bool listenDeviceTable;
	std::vector<unsigned long long> listenIdxs;
	boost::mutex listenMutex;
	http::server::CWebsocketHandler *m_sock;
	bool isStarted;
};

