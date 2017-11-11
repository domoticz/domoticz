#include "stdafx.h"
#include "WebsocketPush.h"
#include "../webserver/WebsocketHandler.h"
#include "../main/mainworker.h"

extern boost::signals2::signal<void(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)> sOnNotificationReceived;


CWebSocketPush::CWebSocketPush(http::server::CWebsocketHandler *sock)
{
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
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CWebSocketPush::OnDeviceReceived, this, _1, _2, _3, _4));
	m_sNotification = sOnNotificationReceived.connect(boost::bind(&CWebSocketPush::OnNotificationReceived, this, _1, _2, _3, _4, _5, _6));
	isStarted = true;
}

void CWebSocketPush::Stop()
{
	if (!isStarted) {
		return;
	}
	isStarted = false;
	ClearListenTable();
	if (m_sConnection.connected()) {
		m_sConnection.disconnect();
	}
	if (m_sNotification.connected()) {
		m_sNotification.disconnect();
	}
}

void CWebSocketPush::ListenTo(const unsigned long long DeviceRowIdx)
{
	boost::unique_lock<boost::mutex>(listenMutex);
	bool bExists = std::find(listenIdxs.begin(), listenIdxs.end(), DeviceRowIdx) != listenIdxs.end();
	if (!bExists) {
		listenIdxs.push_back(DeviceRowIdx);
	}
}

void CWebSocketPush::UnlistenTo(const unsigned long long DeviceRowIdx)
{
	boost::unique_lock<boost::mutex>(listenMutex);
	listenIdxs.erase(std::remove(listenIdxs.begin(), listenIdxs.end(), DeviceRowIdx), listenIdxs.end());
}

void CWebSocketPush::ClearListenTable()
{
	boost::unique_lock<boost::mutex>(listenMutex);
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
	boost::unique_lock<boost::mutex>(listenMutex);
	return std::find(listenIdxs.begin(), listenIdxs.end(), DeviceRowIdx) != listenIdxs.end();
}

void CWebSocketPush::OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	if (!isStarted) {
		return;
	}

	m_sock->OnDeviceChanged(DeviceRowIdx);
	if (WeListenTo(DeviceRowIdx)) {
		// push notification to web socket
	}
}

void CWebSocketPush::OnNotificationReceived(const std::string & Subject, const std::string & Text, const std::string & ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)
{
	if (!isStarted) {
		return;
	}

	// push message to websocket
	m_sock->OnMessage(Subject, Text, ExtraData, Priority, Sound, bFromNotification);
}
