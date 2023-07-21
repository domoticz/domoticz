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
	void ListenTo(uint64_t DeviceRowIdx);
	void UnlistenTo(uint64_t DeviceRowIdx);
	void ClearListenTable();
	void ListenToRoomplan();
	void UnlistenToRoomplan();
	void ListenToDeviceTable();
	void UnlistenToDeviceTable();
	void onRoomplanChanged();
	void onDeviceTableChanged(); // device added, or deleted
	// etc, we need a notification of all changes that need to be reflected in the UI
	bool WeListenTo(uint64_t DeviceRowIdx);

      private:
	void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void OnDeviceUpdate(int m_HwdID, uint64_t DeviceRowIdx);
	void OnNotificationReceived(const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound, bool bFromNotification);
	void OnSceneChange(uint64_t SceneRowIdx, const std::string &SceneName);
	bool listenRoomplan;
	bool listenDeviceTable;
	std::vector<uint64_t> listenIdxs;
	std::mutex listenMutex;
	std::mutex handlerMutex;
	http::server::CWebsocketHandler *m_sock;
	bool isStarted;
};

