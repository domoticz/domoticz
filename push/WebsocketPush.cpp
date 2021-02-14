#include "stdafx.h"
#include "WebsocketPush.h"
#include "../webserver/WebsocketHandler.h"
#include "../main/mainworker.h"

extern boost::signals2::signal<void(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)> sOnNotificationReceived;

CWebSocketPush::CWebSocketPush(http::server::CWebsocketHandler *sock)
{
	m_PushType = PushType::PUSHTYPE_WEBSOCKET;
	listenRoomplan = false;
	listenDeviceTable = false;
	m_sock = sock;
	isStarted = false;
}

void CWebSocketPush::Start()
{
	if (isStarted) {
		return;
	}
	m_sConnection = m_mainworker.sOnDeviceReceived.connect([this](auto id, auto idx, const auto &name, auto rx) { OnDeviceReceived(id, idx, name, rx); });
	m_sDeviceUpdate = m_mainworker.sOnDeviceUpdate.connect([this](auto id, auto idx) { OnDeviceUpdate(id, idx); });
	m_sNotification = sOnNotificationReceived.connect([this](const auto &s, const auto &t, const auto &e, auto p, const auto &sound, auto n) { OnNotificationReceived(s, t, e, p, sound, n); });
	m_sSceneChanged = m_mainworker.sOnSwitchScene.connect([this](auto idx, const auto &name) { OnSceneChange(idx, name); });
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

	isStarted = false;
	ClearListenTable();
}

void CWebSocketPush::ListenTo(const unsigned long long DeviceRowIdx)
{
	std::unique_lock<std::mutex> lock(listenMutex);
	bool bExists = std::find(listenIdxs.begin(), listenIdxs.end(), DeviceRowIdx) != listenIdxs.end();
	if (!bExists) {
		listenIdxs.push_back(DeviceRowIdx);
	}
}

void CWebSocketPush::UnlistenTo(const unsigned long long DeviceRowIdx)
{
	std::unique_lock<std::mutex> lock(listenMutex);
	listenIdxs.erase(std::remove(listenIdxs.begin(), listenIdxs.end(), DeviceRowIdx), listenIdxs.end());
}

void CWebSocketPush::ClearListenTable()
{
	std::unique_lock<std::mutex> lock(listenMutex);
	listenIdxs.clear();
}

void CWebSocketPush::ListenToRoomplan()
{
	listenRoomplan = true;
}

void CWebSocketPush::UnlistenToRoomplan()
{
	listenRoomplan = false;
}

void CWebSocketPush::ListenToDeviceTable()
{
	listenDeviceTable = true;
}

void CWebSocketPush::UnlistenToDeviceTable()
{
	listenDeviceTable = false;
}

void CWebSocketPush::onRoomplanChanged()
{
	if (listenRoomplan) {
		// send notification to web socket
	}
}

void CWebSocketPush::onDeviceTableChanged()
{
	if (listenDeviceTable) {
		// send notification to web socket
	}
}

bool CWebSocketPush::WeListenTo(const unsigned long long DeviceRowIdx)
{
	std::unique_lock<std::mutex> lock(listenMutex);
	return std::find(listenIdxs.begin(), listenIdxs.end(), DeviceRowIdx) != listenIdxs.end();
}

void CWebSocketPush::OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	std::unique_lock<std::mutex> lock(handlerMutex);
	if (!isStarted) {
		return;
	}

	m_sock->OnDeviceChanged(DeviceRowIdx);
	if (WeListenTo(DeviceRowIdx)) {
		// push notification to web socket
	}
}

void CWebSocketPush::OnDeviceUpdate(const int m_HwdID, const unsigned long long DeviceRowIdx)
{
	std::unique_lock<std::mutex> lock(handlerMutex);
	if (!isStarted) {
		return;
	}

	m_sock->OnDeviceChanged(DeviceRowIdx);
	if (WeListenTo(DeviceRowIdx)) {
		// push notification to web socket
	}
}

void CWebSocketPush::OnSceneChange(const unsigned long long SceneRowIdx, const std::string& SceneName)
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
