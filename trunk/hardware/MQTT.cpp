#include "stdafx.h"
#include "MQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

extern "C" {
	#include "../MQTT/MQTTAsync.h"
};

// mosquitto_pub -h 127.0.0.1 -p 1883 -d -t hello/world -m "Hello, MQTT. This is my first message."

#define RETRY_DELAY 30

#define CLIENTID    "Domoticz"
#define TOPIC       "hello/world"
#define QOS         1

MQTT::MQTT(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_IsConnected = false;
#if defined WIN32
	int ret;
	//Init winsock
	WSADATA data;
	WORD version; 

	version = (MAKEWORD(2, 2)); 
	ret = WSAStartup(version, &data); 
	if (ret != 0) 
	{  
		ret = WSAGetLastError(); 

		if (ret == WSANOTINITIALISED) 
		{  
			_log.Log(LOG_ERROR,"MQTT: Winsock could not be initialized!");
		}
	}
#endif
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;

	m_mqtt_client = NULL;
}

MQTT::~MQTT(void)
{
	if (m_mqtt_client)
	{
		MQTTAsync_destroy(&m_mqtt_client);
	}
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool MQTT::StartHardware()
{
	StartHeartbeatThread();

	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_IsConnected=false;

	m_bIsStarted = true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MQTT::Do_Work, this)));
	return (m_thread!=NULL);
}

void mqtt_onDisconnect(void* context, MQTTAsync_successData* response)
{
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_STATUS, "MQTT: Successful disconnection");
	pClient->m_disc_finished = true;
}

void MQTT::StopMQTT()
{
	if (m_mqtt_client)
	{
		if (MQTTAsync_isConnected(m_mqtt_client))
		{
			MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
			disc_opts.onSuccess = mqtt_onDisconnect;
			disc_opts.context = this;
			m_disc_finished = false;
			MQTTAsync_disconnect(m_mqtt_client, &disc_opts);
			while (!m_disc_finished)
			{
#if defined(WIN32)
				Sleep(100);
#else
				usleep(10000L);
#endif 
			}
		}
		MQTTAsync_destroy(&m_mqtt_client);
	}
	m_bIsStarted = false;
}

bool MQTT::StopHardware()
{
	m_stoprequested=true;
	try {
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	if (isConnected())
	{
		try {
			//disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	StopMQTT();

	m_IsConnected = false;
	return true;
}

void mqtt_onSubscribe(void* context, MQTTAsync_successData* response)
{
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_STATUS, "MQTT: Subscribed");
	pClient->m_IsConnected=true;
}

void mqtt_onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_ERROR, "MQTT: Subscribe failed, rc %d", response ? response->code : 0);
	pClient->m_bDoRestart = true;
}

void MQTT::OnConnect()
{
	_log.Log(LOG_STATUS,"MQTT: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	_log.Log(LOG_STATUS, "MQTT: Subscribing to topic: %s", TOPIC);

	opts.onSuccess = mqtt_onSubscribe;
	opts.onFailure = mqtt_onSubscribeFailure;
	opts.context = this;

	//deliveredtoken = 0;

	if ((rc = MQTTAsync_subscribe(m_mqtt_client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		_log.Log(LOG_ERROR, "MQTT: Failed to start subscribe, code: %d", rc);
		m_bDoRestart = true;
		return;
	}
	m_IsConnected = true;
	sOnConnected(this);
}

void MQTT::OnMQTTMessage(char *topicName, int topicLen, void *pMessage)
{
	MQTTAsync_message *message = (MQTTAsync_message*)pMessage;
	if (message->payloadlen < 1)
		return;
	std::string topic = std::string(topicName, topicName + topicLen);
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	_log.Log(LOG_STATUS, "MQTT: Topic: %s, Message: %s", topic.c_str(), qMessage.c_str());

	if (topic.find("MyMQTT/") != std::string::npos)
	{
		//MySensors message
		_log.Log(LOG_STATUS, "MQTT: MySensors message received!");
	}

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
}

void mqtt_connlost(void *context, char *cause)
{
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_ERROR, "MQTT: disconnected");
	pClient->m_bDoRestart = true;
}

int mqtt_msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
	MQTT *pClient = (MQTT*)context;
	pClient->OnMQTTMessage(topicName, topicLen, message);
	return 1;
}

void mqtt_onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_ERROR, "MQTT: connection failed, restarting");
	pClient->m_bDoRestart = true;
}


void mqtt_onConnect(void* context, MQTTAsync_successData* response)
{
	MQTT *pClient = (MQTT*)context;
	pClient->OnConnect();
}

bool MQTT::ConnectInt(const std::string &IPAddress, const unsigned short usIPPort)
{
	StopMQTT();
	m_bDoRestart = false;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

	std::stringstream sstr;
	sstr << "tcp://" << m_szIPAddress << ":" << m_usIPPort;
	std::string szAddress = sstr.str();

	_log.Log(LOG_STATUS, "MQTT: Connecting to %s:%d",m_szIPAddress.c_str(),m_usIPPort);


	if (MQTTAsync_create(&m_mqtt_client, szAddress.c_str(), CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTASYNC_SUCCESS)
		return false;

	MQTTAsync_setCallbacks(m_mqtt_client, this, mqtt_connlost, mqtt_msgarrvd, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = mqtt_onConnect;
	conn_opts.onFailure = mqtt_onConnectFailure;
	conn_opts.connectTimeout = 10;
	conn_opts.context = this;
	int rc;
	if ((rc = MQTTAsync_connect(m_mqtt_client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		_log.Log(LOG_ERROR, "MQTT: Failed to start, return code: %d", rc);
		return false;
	}
	return true;
}

void MQTT::Do_Work()
{
	bool bFirstTime=true;

	while (!m_stoprequested)
	{
		sleep_seconds(1);

		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);


		if (ltime.tm_sec % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			ConnectInt(m_szIPAddress,m_usIPPort);
		}
		else
		{
			time_t atime=time(NULL);
			if ((m_bDoRestart)&&(atime%30==0))
			{
				ConnectInt(m_szIPAddress,m_usIPPort);
			}
		}
	}
	_log.Log(LOG_STATUS,"MQTT: Worker stopped...");
}

void mqtt_onSend(void* context, MQTTAsync_successData* response)
{
//	MQTT *pClient = (MQTT*)context;
}

void MQTT::SendMessage(const std::string &Message)
{
	if (!m_IsConnected)
	{
		return;
	}
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	opts.onSuccess = mqtt_onSend;
	opts.context = this;

	pubmsg.payload = (void*) Message.c_str();
	pubmsg.payloadlen = Message.size();
	pubmsg.qos = QOS;
	pubmsg.retained = 0;

	if (MQTTAsync_sendMessage(m_mqtt_client, TOPIC, &pubmsg, &opts) != MQTTASYNC_SUCCESS)
	{
		_log.Log(LOG_STATUS, "MQTT: Failed to send message: %s", Message.c_str());
	}
}

bool MQTT::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!m_IsConnected)
	{
		return false;
	}
//	write((const unsigned char*)pdata,length);
	return true;
}

