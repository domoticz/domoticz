#include "stdafx.h"
#include "WebsocketPush.h"
#include "../webserver/Websockets.hpp"
#include "../main/mainworker.h"

CWebSocketPush::CWebSocketPush(http::server::CWebsocket *sock)
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
	isStarted = true;
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CWebSocketPush::OnDeviceReceived, this, _1, _2, _3, _4));
}

void CWebSocketPush::Stop()
{
	if (!isStarted) {
		return;
	}
	isStarted = false;
	ClearListenTable();
	if (m_sConnection.connected())
		m_sConnection.disconnect();
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
	m_sock->OnDeviceChanged(DeviceRowIdx);
	if (WeListenTo(DeviceRowIdx)) {
		// push notification to web socket
	}
}
