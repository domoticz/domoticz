#pragma once
#include "BasePush.h"

namespace http {
	namespace server {
		class CWebsocketHandler;
	} // namespace server
} // namespace http

class CWebSocketPush : public CBasePush
{
public:
	explicit CWebSocketPush(http::server::CWebsocketHandler *sock);
	void Start();
	void Stop();
	void ListenTo(unsigned long long DeviceRowIdx);
	void UnlistenTo(unsigned long long DeviceRowIdx);
	void ClearListenTable();
	void ListenToRoomplan();
	void UnlistenToRoomplan();
	void ListenToDeviceTable();
	void UnlistenToDeviceTable();
	void onRoomplanChanged();
	void onDeviceTableChanged(); // device added, or deleted
	// etc, we need a notification of all changes that need to be reflected in the UI
	bool WeListenTo(unsigned long long DeviceRowIdx);

      private:
	void OnDeviceReceived(int m_HwdID, unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void OnDeviceUpdate(int m_HwdID, unsigned long long DeviceRowIdx);
	void OnNotificationReceived(const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound, bool bFromNotification);
	void OnSceneChange(unsigned long long SceneRowIdx, const std::string &SceneName);
	bool listenRoomplan;
	bool listenDeviceTable;
	std::vector<unsigned long long> listenIdxs;
	std::mutex listenMutex;
	std::mutex handlerMutex;
	http::server::CWebsocketHandler *m_sock;
	bool isStarted;
};

