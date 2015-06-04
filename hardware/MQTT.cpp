#include "stdafx.h"
#include "MQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../json/json.h"

extern "C" {
	#include "../MQTT/MQTTAsync.h"
};


boost::mutex m_mqtt_mutex;

// mosquitto_pub -h 127.0.0.1 -p 1883 -d -t hello/world -m "Hello, MQTT. This is my first message."

#define RETRY_DELAY 30

#define CLIENTID	"Domoticz"
#define TOPIC_OUT	"domoticz/out"
#define TOPIC_IN	"domoticz/in"
#define QOS         1

MQTT::MQTT(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_bDoReconnect = false;
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
	m_bDoReconnect = false;

	//force connect the next first time
	m_IsConnected=false;

	m_bIsStarted = true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MQTT::Do_Work, this)));
	return (m_thread!=NULL);
}

void mqtt_onDisconnect(void* context, MQTTAsync_successData* response)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
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
		m_mqtt_client = NULL;
	}
	m_bIsStarted = false;
}

bool MQTT::StopHardware()
{
	StopHeartbeatThread();
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
	if (m_sConnection.connected())
		m_sConnection.disconnect();
	m_IsConnected = false;
	return true;
}

void mqtt_onSubscribe(void* context, MQTTAsync_successData* response)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_STATUS, "MQTT: Subscribed");
	pClient->m_IsConnected=true;
}

void mqtt_onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
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

	_log.Log(LOG_STATUS, "MQTT: Subscribing to topic: %s", TOPIC_OUT);

	opts.onSuccess = mqtt_onSubscribe;
	opts.onFailure = mqtt_onSubscribeFailure;
	opts.context = this;

	//deliveredtoken = 0;

	if ((rc = MQTTAsync_subscribe(m_mqtt_client, TOPIC_OUT, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		_log.Log(LOG_ERROR, "MQTT: Failed to start subscribe, code: %d", rc);
		m_bDoRestart = true;
		return;
	}

	_log.Log(LOG_STATUS, "MQTT: Subscribing to topic: %s", TOPIC_IN);

	if ((rc = MQTTAsync_subscribe(m_mqtt_client, TOPIC_IN, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		_log.Log(LOG_ERROR, "MQTT: Failed to start subscribe, code: %d", rc);
		m_bDoRestart = true;
		return;
	}

	m_IsConnected = true;
	sOnConnected(this);
	m_sConnection=m_mainworker.sOnDeviceReceived.connect(boost::bind(&MQTT::SendDeviceInfo, this, _1, _2, _3, _4));
}

void MQTT::OnMQTTMessage(char *topicName, int topicLen, void *pMessage)
{
	MQTTAsync_message *message = (MQTTAsync_message*)pMessage;
	std::string topic = std::string(topicName, topicName + topicLen);

	if (topic == TOPIC_OUT)
	{
		//ignore our own outgoing messages
		return;
	}

	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);

	_log.Log(LOG_STATUS, "MQTT: Topic: %s, Message: %s", topic.c_str(), qMessage.c_str());

	if (qMessage.empty())
		return;

	if (topic.find("MyMQTT") != std::string::npos)
	{
		//MySensors message
		ProcessMySensorsMessage(qMessage);
		return;
	}
	else if (topic == TOPIC_IN)
	{
		Json::Value root;

		Json::Reader jReader;
		bool ret = jReader.parse(qMessage, root);
		if (!ret)
		{
			_log.Log(LOG_ERROR, "MQTT: Invalid data received!");
			return;
		}
		bool bValid = true;
		if (root["idx"].empty())
		{
			bValid = false;
		}
		if (root["nvalue"].empty())
		{
			bValid = false;
		}
		if (root["svalue"].empty())
		{
			bValid = false;
		}
		if (!bValid)
		{
			_log.Log(LOG_ERROR, "MQTT: Invalid data received! (Missing idx,nvalue,svalue)");
			return;
		}
		int idx = root["idx"].asInt();
		int nvalue = root["nvalue"].asInt();
		std::string svalue = root["svalue"].asString();

		int signallevel = 12;
		int batterylevel = 255;

		if (!m_mainworker.UpdateDevice(idx, nvalue, svalue, signallevel, batterylevel))
		{
			_log.Log(LOG_ERROR, "MQTT: unknown idx!");
			return;
		}
	}
}

void mqtt_connlost(void *context, char *cause)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_ERROR, "MQTT: disconnected, restarting");
	pClient->m_bDoReconnect = true;
}

int mqtt_msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	MQTT *pClient = (MQTT*)context;
	pClient->OnMQTTMessage(topicName, topicLen, message);
	return 1;
}

void mqtt_onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	MQTT *pClient = (MQTT*)context;
	_log.Log(LOG_ERROR, "MQTT: connection failed, restarting");
	pClient->m_bDoReconnect = true;
}


void mqtt_onConnect(void* context, MQTTAsync_successData* response)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	MQTT *pClient = (MQTT*)context;
	pClient->OnConnect();
}

bool MQTT::ConnectInt()
{
	StopMQTT();
	m_bDoRestart = false;
	std::stringstream sstr;
	sstr << "tcp://" << m_szIPAddress << ":" << m_usIPPort;
	std::string szAddress = sstr.str();



	if (MQTTAsync_create(&m_mqtt_client, m_szIPAddress.c_str(), CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTASYNC_SUCCESS)
		return false;

	MQTTAsync_setCallbacks(m_mqtt_client, this, mqtt_connlost, mqtt_msgarrvd, NULL);
	return ConnectIntEx();
}

bool MQTT::ConnectIntEx()
{
	m_bDoReconnect = false;
	_log.Log(LOG_STATUS, "MQTT: Connecting to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
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
			ConnectInt();
		}
		else
		{
			time_t atime=time(NULL);
			if (atime%30==0)
			{
				if (m_bDoRestart)
					ConnectInt();
				else if (m_bDoReconnect)
					ConnectIntEx();
			}
		}
	}
	_log.Log(LOG_STATUS,"MQTT: Worker stopped...");
}

void mqtt_onSend(void* context, MQTTAsync_successData* response)
{
//	MQTT *pClient = (MQTT*)context;
}

void MQTT::SendMessage(const std::string &Topic, const std::string &Message)
{
	try {
		if (!m_IsConnected)
		{
			_log.Log(LOG_STATUS, "MQTT: Not Connected, failed to send message: %s", Message.c_str());
			return;
		}
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
		MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
		opts.onSuccess = mqtt_onSend;
		opts.context = this;

		pubmsg.payload = (void*)Message.c_str();
		pubmsg.payloadlen = Message.size();
		pubmsg.qos = QOS;
		pubmsg.retained = 0;

		if (MQTTAsync_sendMessage(m_mqtt_client, Topic.c_str(), &pubmsg, &opts) != MQTTASYNC_SUCCESS)
		{
			_log.Log(LOG_ERROR, "MQTT: Failed to send message: %s", Message.c_str());
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "MQTT: Failed to send message: %s", Message.c_str());
	}
}

void MQTT::WriteInt(const std::string &sendStr)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	if (sendStr.size() < 2)
		return;
	//string the return and the end
	std::string sMessage = std::string(sendStr.begin(), sendStr.begin() + sendStr.size()-1);
	SendMessage("MyMQTT", sMessage);
}

void MQTT::ProcessMySensorsMessage(const std::string &MySensorsMessage)
{
	m_bufferpos = MySensorsMessage.size();
	memcpy(&m_buffer, MySensorsMessage.c_str(), m_bufferpos);
	m_buffer[m_bufferpos] = 0;
	ParseLine();
}

void MQTT::SendDeviceInfo(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	if (m_mqtt_client == NULL)
		return;
	if (!MQTTAsync_isConnected(m_mqtt_client))
		return;
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT DeviceID, Unit, Name, [Type], SubType, nValue, sValue, SwitchType FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << DeviceRowIdx << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() > 0)
	{
		std::vector<std::string> sd = result[0];
		std::string did = sd[0];
		int dunit = atoi(sd[1].c_str());
		std::string name = sd[2];
		int dType = atoi(sd[3].c_str());
		int dSubType = atoi(sd[4].c_str());
		int nvalue = atoi(sd[5].c_str());
		std::string svalue = sd[6];


		Json::Value root;

		root["idx"] = DeviceRowIdx;
		root["id"] = did;
		root["unit"] = dunit;
		root["name"] = name;
		root["dtype"] = RFX_Type_Desc(dType,1);
		root["stype"] = RFX_Type_SubType_Desc(dType, dSubType);
		root["nvalue"] = nvalue;

		//give all svalues separate
		std::vector<std::string> strarray;
		StringSplit(svalue, ";", strarray);

		std::vector<std::string>::const_iterator itt;
		int sIndex = 1;
		for (itt = strarray.begin(); itt != strarray.end(); ++itt)
		{
			szQuery.str("");
			szQuery.clear();
			szQuery << "svalue" << sIndex;
			root[szQuery.str()] = *itt;
			sIndex++;
		}
		SendMessage(TOPIC_OUT, root.toStyledString());
	}
}

