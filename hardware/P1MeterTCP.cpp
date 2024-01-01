#include "stdafx.h"
#include "P1MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"

#ifdef _DEBUG
//#define DEBUG_P1_R
#endif

#ifdef DEBUG_P1_R
//Belgium
const char* szP1Test = R"p1_test(/FLU5\253770234_A

0-0:96.1.4(50213)
0-0:96.1.1(3153414731313030303037313930)
0-0:1.0.0(190905142315S)
1-0:1.8.1(000244.844*kWh)
1-0:1.8.2(000226.027*kWh)
1-0:2.8.1(000139.553*kWh)
1-0:2.8.2(000045.390*kWh)
0-0:96.14.0(0001)
1-0:1.7.0(00.000*kW)
1-0:2.7.0(00.198*kW)
1-0:32.7.0(240.0*V)
1-0:31.7.0(002*A)
0-0:96.3.10(1)
0-0:17.0.0(999.9*kW)
1-0:31.4.0(999*A)
0-0:96.13.0()
0-1:24.1.0(003)
0-1:96.1.1(37464C4F32313139303137303532)
0-1:24.4.0(1)
0-1:24.2.3(190905142001S)(00071.724*m3)
!0F77
)p1_test";


const char* szP1TestWater = R"p1_test_water(/FLU5\253769484_A

0-0:96.1.4(50216)
0-0:96.1.1(3153414733313030353733313933)
0-0:1.0.0(220812104627S)
1-0:1.8.1(000012.755*kWh)
1-0:1.8.2(000005.763*kWh)
1-0:2.8.1(000000.159*kWh)
1-0:2.8.2(000000.000*kWh)
0-0:96.14.0(0001)
1-0:1.7.0(00.123*kW)
1-0:2.7.0(00.000*kW)
1-0:21.7.0(00.009*kW)
1-0:41.7.0(00.006*kW)
1-0:61.7.0(00.107*kW)
1-0:22.7.0(00.000*kW)
1-0:42.7.0(00.000*kW)
1-0:62.7.0(00.000*kW)
1-0:32.7.0(229.1*V)
1-0:52.7.0(231.5*V)
1-0:72.7.0(233.1*V)
1-0:31.7.0(000.09*A)
1-0:51.7.0(000.03*A)
1-0:71.7.0(000.82*A)
0-0:96.3.10(1)
0-0:17.0.0(999.9*kW)
1-0:31.4.0(999*A)
0-0:96.13.0()
0-1:24.1.0(003)
0-1:96.1.1(37464C4F32313232303935333931)
0-1:24.4.0(1)
0-1:24.2.3(220812104535S)(00001.326*m3)
0-2:24.1.0(007)
0-2:96.1.1(3853455430303030323731323938)
0-2:24.2.1(220812104503S)(00000.302*m3)
!25FD
)p1_test_water";
#endif

//0-1:24.1.0(003) = gas (3)
//0-2:24.1.0(007) = water (7)

P1MeterTCP::P1MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const bool disable_crc, const int ratelimit, const std::string& DecryptionKey):
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort)
{
	m_HwdID = ID;
	m_bDisableCRC = disable_crc;
	m_ratelimit = ratelimit;

	if (!DecryptionKey.empty())
	{
		m_bIsEncrypted = true;
		m_szHexKey = HexToBytes(DecryptionKey);
	}
}

bool P1MeterTCP::StartHardware()
{
	RequestStart();

	m_bIsStarted = true;

#ifdef DEBUG_P1_R
	ParseP1Data((const uint8_t*)szP1TestWater, static_cast<int>(strlen(szP1TestWater)), m_bDisableCRC, m_ratelimit);
#endif

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}


bool P1MeterTCP::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}


void P1MeterTCP::Do_Work()
{
	int sec_counter = 0;
	Log(LOG_STATUS, "attempt connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress, m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
	}
	terminate();

	Log(LOG_STATUS, "TCP/IP Worker stopped...");
}


bool P1MeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	// read only device - just return true or false depending on connection status
	return isConnected();
}


void P1MeterTCP::OnConnect()
{
	// reset all values and buffers - they may contain invalid data
	Init();
	Log(LOG_STATUS, "connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	if (m_bDisableCRC)
	{
		Log(LOG_STATUS, "P1 Smart Meter: CRC validation disabled through hardware control");
	}
}


void P1MeterTCP::OnDisconnect()
{
	Log(LOG_STATUS, "disconnected");
}


void P1MeterTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseP1Data((const unsigned char*)pData, static_cast<int>(length), m_bDisableCRC, m_ratelimit);
}


void P1MeterTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		Log(LOG_ERROR, "Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		Log(LOG_STATUS, "Connection reset!");
	}
	else
	{
		Log(LOG_ERROR, "%s", error.message().c_str());
	}
}

