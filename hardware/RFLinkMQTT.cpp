//------------------------------------------------------------------------------
// Add hardware plugin:
//  https://www.domoticz.com/wiki/Developing_a_hardware_plugin
// + Add type to herdware type selection function:
// RFXNames.cpp
// IsSerialDevice / IsNetworkDevice
// -----------------------------------------------------------------------------
// RFLINK MQTT gateway
// Created 2018.07.
// Use this HW you need a RFLINK serial to MQTT proxy
// https://github.com/pagocs/esp32-rflinkmqttgateway
// -----------------------------------------------------------------------------
// Main goals
// This hardware plugin is allow to use one RFLINK gateway hardware from multiple
// domoticz system.
//
// Fail safety
// You can define multiple MQTT target in hw config Remote Address settings
// The list should contains ";" separated domain or IP addresses
// Of course it could be use with single target
// -----------------------------------------------------------------------------

#include "stdafx.h"
#include "RFLinkMQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include <iostream>
#include <inttypes.h>
#include <json/json.h>

// -----------------------------------------------------------------------------
// TODO: Add to doumentetion
// main/mainworker.cpp
// case HTYPE_RFLINKMQTT:726
// 	pHardware = new CRFLinkMQTT(ID, Address , Port , Username , Password );
// Webserver.cpp: line 1499
// HardwareCOntroller.js
// Hardware settings panel: UpdateHardwareParamControls
// Add hardware to main/WebServer.cpp CWebServer::Cmd_AddHardware
// If your calss is inherited from other domoticz calss
// Add your calss as frend.

// TODO:
// www/app/HardwareController.js:RefreshHardwareTable
// At the beginning there add the SerialName nd etc if it is necesarry


#define TOPIC_OUT	"rflink/out"
#define TOPIC_IN	"rflink/in"
#define QOS         1

namespace
{
	constexpr std::array<const char *, 3> szTLSVersions{
		"tlsv1",   //
		"tlsv1.1", //
		"tlsv1.2", //
	};
} // namespace

extern std::string szRandomUUID;

// -----------------------------------------------------------------------------

CRFLinkMQTT::CRFLinkMQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort ,
						const std::string &Username, const std::string &Password , const std::string &CAfilenameExtra,
						const int TLS_Version, const int PublishScheme, const bool Multidomonodesync ):
	mosqdz::mosquittodz((std::string("RFLINKMQTT") + std::string(GenerateUUID())).c_str()),
	m_szIPAddressList(IPAddress),
	m_usIPPort(usIPPort),
	m_UserName(Username),
	m_Password(Password),
	m_bMultidomonodesync( Multidomonodesync )
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_IsConnected = false;
	m_bDoReconnect = false;
	m_TopicIn = TOPIC_IN;
	m_TopicOut = TOPIC_OUT;
    m_cmdacktimeout = 2;      // override m_bTXokay wait timeout
	m_retrycntr = RFLINK_RETRY_DELAY;
    m_syncid = (unsigned long)rand();
	m_lastmsgTime = mytime(nullptr);

	std::vector<std::string> strarray;
	StringSplit(CAfilenameExtra, ";", strarray);
	if (!strarray.empty())
	{
		m_CAFilename = strarray[0];
	}

	// _log.Debug(DEBUG_HARDWARE, ">>> RFLINK MQTT: user: %s password: %s extra: %s" , m_UserName.c_str() , m_Password.c_str() , m_CAFilename.c_str() );
	_log.Debug(DEBUG_HARDWARE, ">>> RFLINK MQTT: m_bMultidomonodesync: %s" , m_bMultidomonodesync == true ? "true" : "false" );

	m_TLS_Version = (TLS_Version < 3) ? TLS_Version : 0; // see szTLSVersions

	selectNextIPAdress();

	// Init MQTT
	mosqdz::lib_init();

	threaded_set(true);

	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: Device initiated...");
}

CRFLinkMQTT::~CRFLinkMQTT(void)
{
	mosqdz::lib_cleanup();
}

// this function is select the MQTT target IP round-robin from the defined pool
void CRFLinkMQTT::selectNextIPAdress( void )
{
	// Scan the IP address list and select the next oneto connect MQTT server
	std::vector<std::string>	ipadresses;
    StringSplit(m_szIPAddressList, ";", ipadresses);
	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: available IPs (%d): %s" , (int)ipadresses.size() , m_szIPAddressList.c_str() );

	if( ipadresses.size() > 1 )
	{
		// there is more IP available
		std::string		ip = ipadresses.front();
		for ( auto it = ipadresses.begin(); it != ipadresses.end(); ++it)
		{
			if( ip.empty() )
			{
				ip = *it;
			}
			else if( m_szIPAddress == *it && std::next(it) != ipadresses.end() )
			{
				ip.clear();
			}
		}
		m_szIPAddress = ip;
	}
	else
	{
		// Just one IP defined
		m_szIPAddress = m_szIPAddressList;
	}

	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: Set MQTT target to: %s" , m_szIPAddress.c_str() );

}

bool CRFLinkMQTT::StartHardware()
{
	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: StartHardware started");

	RequestStart();

	// force connect the next first time
	m_IsConnected = false;
	m_bIsStarted=true;

	// Start worker thread
	m_thread = std::make_shared<std::thread>(&CRFLinkMQTT::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	StartHeartbeatThread();
	return (m_thread != nullptr);
}

void CRFLinkMQTT::StopMQTT()
{
	disconnect();
	m_bIsStarted = false;
	m_IsConnected = false;
}


bool CRFLinkMQTT::StopHardware()
{
	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: StopHardware started");

	StopHeartbeatThread();
	try {
		if (m_thread)
		{
			RequestStop();
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
		_log.Log(LOG_ERROR, ">>> RFLINK MQTT: Something awkward is happened...");
	}

	if (m_sDeviceReceivedConnection.connected())
		m_sDeviceReceivedConnection.disconnect();
	if (m_sSwitchSceneConnection.connected())
		m_sSwitchSceneConnection.disconnect();

	m_IsConnected = false;
	m_bIsStarted=false;
	return true;
}

void CRFLinkMQTT::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: Subscribed");
	m_IsConnected = true;
}

void CRFLinkMQTT::on_connect(int rc)
{
	/* rc=
	** 0 - success
	** 1 - connection refused(unacceptable protocol version)
	** 2 - connection refused(identifier rejected)
	** 3 - connection refused(broker unavailable)
	*/

	m_rfbufferpos = 0;

	if (rc == 0)
	{
		if (m_IsConnected)
		{
			_log.Log(LOG_STATUS, ">>> RFLINK MQTT: re-connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		}
		else
		{
			_log.Log(LOG_NORM, ">>> RFLINK MQTT: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
		}
		if (!m_TopicIn.empty())
		{
			subscribe(NULL, m_TopicIn.c_str());
		}
	}
	else
	{
		_log.Log(LOG_ERROR, ">>> RFLINK MQTT: Connection failed!, restarting (rc=%d)",rc);
		m_bDoReconnect = true;
	}
}

void CRFLinkMQTT::on_message(const struct mosquitto_message *message)
{
	// Message on MQTT
	std::string topic = message->topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	_log.Log(LOG_NORM, ">>> RFLINK MQTT: Topic: %s, Message: %s", topic.c_str(), qMessage.c_str());
	// Add newwline to end of the buffer
	qMessage += '\n';

	if (topic != m_TopicIn)
		return;

    std::stringstream   filter;
    filter << "SYNCID=" << std::hex << m_syncid;

    //_log.Log(LOG_NORM, ">>> RFLINK MQTT: Checking syncback (%s)", filter.str().c_str());
    if ( qMessage.find( filter.str() ) != std::string::npos )
    {
        _log.Log(LOG_NORM, ">>> RFLINK MQTT: Syncback message filtered..." );
        return;
    }

	// Filter the duplicated packets
	if( qMessage.length() > 5 )
	{
		// 20;77;EV1527;ID=0f7499;SWITCH=04;CMD=ON;
		//      ^
		size_t seqnumend = qMessage.find( ";" , 3 );

		if( seqnumend != std::string::npos && seqnumend > 3 )
		{
			struct timeval tnow;
			std::string cpofmsg = qMessage;

			// 20;77;EV1527;ID=0f7499;SWITCH=04;CMD=ON;
			//    ^^^
			// Remove the sequence number from payload
			cpofmsg.erase( 3 , seqnumend-3 );
			// Calculate a CRC value for payload comparision -- Crc32 defined in Hhelper.cpp
			unsigned int crc = Crc32( 0 ,(const uint8_t*) cpofmsg.c_str() , cpofmsg.length() );

			// Get time in millisec for sub second comparision
			gettimeofday(&tnow, nullptr);
			time_t msecs = (tnow.tv_sec * 1000) + (tnow.tv_usec / 1000);

			// _log.Log(LOG_NORM, ">>> RFLINK MQTT: Message to chksum: %s CRC: %x time diff: %ld" , cpofmsg.c_str() , crc , msecs-m_lastmsgTime );

			if( m_lastmsgCRC == crc && (msecs-m_lastmsgTime) < 420 )
			{
				// if the CRC is equal in the specified period it should be ignored
				_log.Log(LOG_NORM, ">>> RFLINK MQTT: SKIP duplicated payload!" );
				return;
			}

			m_lastmsgCRC = crc;
			m_lastmsgTime = msecs;
		}
	}

	// Parse  payload as RFLINK serial packet
	ParseData((const char*)qMessage.c_str(),qMessage.length());

}

void CRFLinkMQTT::on_disconnect(int rc)
{
	if (rc != 0)
	{
		if (!IsStopRequested(0))
		{
			// MOSQ_ERR_CONN_REFUSED = 5
			if( rc == MOSQ_ERR_CONN_REFUSED )
			{
				_log.Log(LOG_ERROR, ">>> RFLINK MQTT: disconnected, Invalid Username/Password (rc=%d)", rc);
			}
			else if( rc == MOSQ_ERR_ERRNO )
			{
				// // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// _log.Log(LOG_ERROR, ">>> RFLINK MQTT: disconnected, !!! 4.11554 workaround !!! ERRNO: %d)", errno );
				// // WORKAROUND
				// // 2019.12.08. After MQTT replaced in domoticz I experienced
				// // this error very frequently. This error is causing miss behave of devices!
				// // If in this case do nothing DOMOTICZ will quickly recconect
				// // and looks like everything work perfectly
				// return;
				// // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				_log.Log(LOG_ERROR, ">>> RFLINK MQTT: MOSQ_ERR_ERRNO detected with value: %d)", errno );
			}
			else
			{
				_log.Log(LOG_ERROR, ">>> RFLINK MQTT: disconnected, restarting (rc=%d)", rc);
			}
			m_bDoReconnect = true;
		}
	}
}

bool CRFLinkMQTT::ConnectInt()
{
	StopMQTT();
	return ConnectIntEx();
}

bool CRFLinkMQTT::ConnectIntEx()
{
	m_bDoReconnect = false;
	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: Connecting to %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	int rc;
	int keepalive = 60;

	// FIXME: CA support not implemented yet
	if (!m_CAFilename.empty()){
		rc = tls_set(m_CAFilename.c_str());

		if ( rc != MOSQ_ERR_SUCCESS)
		{
			_log.Log(LOG_ERROR, ">>> RFLINK MQTT:: Failed enabling TLS mode, return code: %d (CA certificate: '%s')", rc, m_CAFilename.c_str());
			return false;
		}
		else
		{
			_log.Log(LOG_STATUS, ">>> RFLINK MQTT:: enabled TLS mode");
		}
	}

	rc = username_pw_set((!m_UserName.empty()) ? m_UserName.c_str() : NULL, (!m_Password.empty()) ? m_Password.c_str() : NULL);
	rc = connect(m_szIPAddress.c_str(), m_usIPPort, keepalive);

	if ( rc != MOSQ_ERR_SUCCESS)
	{
		_log.Log(LOG_ERROR, ">>> RFLINK MQTT: Failed to start, return code: %d (Check IP/Port)", rc);
		// Try to another MQTT server if it is available
		selectNextIPAdress();
		m_bDoReconnect = true;
		return false;
	}
	return true;
}

void CRFLinkMQTT::Do_Work()
{
	bool bFirstTime=true;
	int msec_counter = 0;
	int sec_counter = 0;
	_log.Log(LOG_STATUS, ">>> RFLINK MQTT: main loop started");

	while (!IsStopRequested(100))
	{
		if (!bFirstTime)
		{
			try
			{
				int rc = loop();
				if (rc)
				{
					if (rc != MOSQ_ERR_NO_CONN)
					{
						if (!IsStopRequested(0))
						{
							if (!m_bDoReconnect)
							{
								_log.Log(LOG_STATUS, ">>> RFLINK MQTT: trying to reconnect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
								reconnect();
							}
						}
					}
				}
			}
			catch (const std::exception &)
			{
				if (!IsStopRequested(0))
				{
					if (!m_bDoReconnect)
					{
						reconnect();
					}
				}
			}
		}

		msec_counter++;
		if (msec_counter == 10)
		{
			msec_counter = 0;
			sec_counter++;

			if (sec_counter % 12 == 0)
			{
				m_LastHeartbeat=mytime(nullptr);
			}

			if (bFirstTime)
			{
				bFirstTime = false;
				ConnectInt();
			}
			else
			{
				if (sec_counter % 30 == 0)
				{
					if (m_bDoReconnect)
						ConnectIntEx();
				}
				if (isConnected() && sec_counter % 10 == 0)
				{
					SendHeartbeat();
				}
			}
		}
	}

	clear_callbacks();

	if (isConnected())
		disconnect();

	_log.Log(LOG_STATUS,">>> RFLINK MQTT: Worker stopped...");
}

void CRFLinkMQTT::SendHeartbeat()
{
	// not necessary for normal MQTT servers
}

void CRFLinkMQTT::SendMessage(const std::string &Topic, const std::string &Message)
{
	try {
		if (!m_IsConnected)
		{
			_log.Log(LOG_ERROR, ">>> RFLINK MQTT: Not Connected, failed to send message: %s:%s", Topic.c_str(), Message.substr(0, Message.size()-1).c_str());
			return;
		}
        // Add Syncbak filter ID to the message
        std::string  msg = Message;
        std::stringstream   sstr;
        if( msg.back() == '\n' )
        {
            msg.erase(msg.size() - 1);
        }
        if( msg.back() != ';' )
        {
            msg += ";";
        }

		// The SYNCID is used for filtering the "sync back" packet what used the
		// gateway firmware (https://github.com/pagocs/esp32-rflinkmqttgateway)
		// for syncing the RFLINK commands between multiple Domoticz instances.
		// This option must be disabled if you are want to use different gateway
		// firmware. For example: https://github.com/seb821/espRFLinkMQTT

		if( m_bMultidomonodesync )
		{
			sstr << msg << "SYNCID=" << std::hex << m_syncid << ";\n";
	        msg = sstr.str();
		}
		else
		{
			    msg += "\n";
		}
		//_log.Log(LOG_STATUS, ">>> RFLINK MQTT::Publish message %s:%s", Topic.c_str() , Message.substr(0, Message.size()-1).c_str());
        _log.Log(LOG_NORM, ">>> RFLINK MQTT::Publish message %s:%s", Topic.c_str() , msg.substr(0, msg.size()-1).c_str());
        publish(NULL, Topic.c_str(), msg.size(), msg.c_str());
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, ">>> RFLINK MQTT: Failed to send message: %s::%s", Topic.c_str() , Message.c_str());
	}
}

bool CRFLinkMQTT::WriteInt(const std::string &sendString)
{

	if (sendString.size() < 2)
		return false;
	//string the return and the end
	std::string sMessage = std::string(sendString.begin(), sendString.begin() + sendString.size());
	_log.Debug(DEBUG_HARDWARE, ">>> RFLINK MQTT::WRiteInt %s", sMessage.substr(0, sMessage.size()-1).c_str());
	SendMessage(m_TopicOut, sMessage);
	return false;
}
