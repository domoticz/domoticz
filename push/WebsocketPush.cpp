#include "stdafx.h"
#include "WebsocketPush.h"
#include "../webserver/WebsocketHandler.h"
#include "../main/mainworker.h"
#include "../main/Logger.h"

extern boost::signals2::signal<void(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)> sOnNotificationReceived;

CWebSocketPush::CWebSocketPush(http::server::CWebsocketHandler *sock)
{
	m_PushType = PushType::PUSHTYPE_WEBSOCKET;
	m_sock = sock;
	isStarted = false;
}

void CWebSocketPush::Start()
{
	if (isStarted) {
		return;
	}
	m_sConnection = m_mainworker.sOnDeviceReceived.connect([this](auto id, auto idx, auto &&name, auto rx) { OnDeviceReceived(id, idx, name, rx); });
	m_sDeviceUpdate = m_mainworker.sOnDeviceUpdate.connect([this](auto id, auto idx) { OnDeviceUpdate(id, idx); });
	m_sNotification = sOnNotificationReceived.connect([this](auto &&s, auto &&t, auto &&e, auto p, auto &&sound, auto n) { OnNotificationReceived(s, t, e, p, sound, n); });
	m_sSceneChanged = m_mainworker.sOnSwitchScene.connect([this](auto idx, auto &&name) { OnSceneChange(idx, name); });

	_log.sOnLogMessage.connect(this, &CWebSocketPush::OnLogMessage, &m_sLogMessage);

	isStarted = true;
}

void CWebSocketPush::Stop()
{
	if (!isStarted) 
		return;

	std::unique_lock<std::mutex> lock(handlerMutex);

	if (m_sConnection.connected())
		m_sConnection.disconnect();

	if (m_sDeviceUpdate.connected())
		m_sDeviceUpdate.disconnect();

	if (m_sNotification.connected())
		m_sNotification.disconnect();

	if (m_sSceneChanged.connected())
		m_sSceneChanged.disconnect();

	m_sLogMessage.disconnect();

	isStarted = false;
}

void CWebSocketPush::OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	std::unique_lock<std::mutex> lock(handlerMutex);
	if (!isStarted) {
		return;
	}

	m_sock->OnDeviceChanged(DeviceRowIdx);
}

void CWebSocketPush::OnDeviceUpdate(const int m_HwdID, const uint64_t DeviceRowIdx)
{
	std::unique_lock<std::mutex> lock(handlerMutex);
	if (!isStarted) {
		return;
	}

	m_sock->OnDeviceChanged(DeviceRowIdx);
}

void CWebSocketPush::OnSceneChange(const uint64_t SceneRowIdx, const std::string& SceneName)
{
	std::unique_lock<std::mutex> lock(handlerMutex);
	if (!isStarted) {
		return;
	}
	m_sock->OnSceneChanged(SceneRowIdx);
}

void CWebSocketPush::OnNotificationReceived(const std::string & Subject, const std::string & Text, const std::string & ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)
{
	std::unique_lock<std::mutex> lock(handlerMutex);
	if (!isStarted) {
		return;
	}

	// push message to websocket
	m_sock->SendNotification(Subject, Text, ExtraData, Priority, Sound, bFromNotification);
}

void CWebSocketPush::OnLogMessage(const _eLogLevel level, const std::string& sLogline)
{
	std::unique_lock<std::mutex> lock(logMutex);
	if (!isStarted) {
		return;
	}
	m_sock->SendLogMessage(static_cast<int>(level), sLogline);
}
