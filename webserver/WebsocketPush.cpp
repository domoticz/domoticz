#include "stdafx.h"
#include "WebsocketPush.h"
#include "../main/mainworker.h"

CWebSocketPush::CWebSocketPush()
{
}

void CWebSocketPush::Start()
{
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CWebSocketPush::OnDeviceReceived, this, _1, _2, _3, _4));
}

void CWebSocketPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
}

void CWebSocketPush::onRoomplanChanged()
{
}

void CWebSocketPush::onDeviceTableChanged()
{
}

void CWebSocketPush::OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
}
