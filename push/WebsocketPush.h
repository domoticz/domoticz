#pragma once
#include "BasePush.h"
#include "../main/lsignal.h"

enum _eLogLevel : uint32_t;

namespace http {
	namespace server {
		class CDomoticzWebsocketHandler;
	} // namespace server
} // namespace http

#include "../main/StoppableTask.h"

class CWebSocketPush : public CBasePush, public StoppableTask
{
public:
	explicit CWebSocketPush(http::server::CDomoticzWebsocketHandler *sock);
	void Start();
	void Stop();
	void onDeviceTableChanged(); // device added, or deleted
private:
	void OnDeviceReceived(int HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void OnDeviceUpdate(int m_HwdID, uint64_t DeviceRowIdx);
	void OnNotificationReceived(const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound, bool bFromNotification);
	void OnSceneChange(uint64_t SceneRowIdx, const std::string &SceneName);
	void OnLogMessage(const _eLogLevel level, const std::string& sLogline);

	std::mutex handlerMutex;
	http::server::CDomoticzWebsocketHandler *m_sock;
	bool isStarted;

	lsignal::slot m_sLogMessage;

};

