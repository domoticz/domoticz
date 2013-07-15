#include "stdafx.h"
#include "Razberry.h"

#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <vector>
#include <ctype.h>

#include "../httpclient/HTTPClient.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"

#pragma warning(disable: 4996)

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
//	#define DEBUG_ZWAVE_INT
#endif

bool isInt(std::string s)
{
	for(size_t i = 0; i < s.length(); i++){
		if(!isdigit(s[i]))
			return false;
	}
	return true;
}

static std::string readInputTestFile( const char *path )
{
	FILE *file = fopen( path, "rb" );
	if ( !file )
		return std::string("");
	fseek( file, 0, SEEK_END );
	long size = ftell( file );
	fseek( file, 0, SEEK_SET );
	std::string text;
	char *buffer = new char[size+1];
	buffer[size] = 0;
	if ( fread( buffer, 1, size, file ) == (unsigned long)size )
		text = buffer;
	fclose( file );
	delete[] buffer;
	return text;
}

CRazberry::CRazberry(const int ID, const std::string ipaddress, const int port, const std::string username, const std::string password)
{
	m_HwdID=ID;
	m_ipaddress=ipaddress;
	m_port=port;
	m_username=username;
	m_password=password;
}


CRazberry::~CRazberry(void)
{
}

bool CRazberry::StartHardware()
{
	m_bInitState=true;
	m_updateTime=0;
	m_controllerID=0;
	m_stoprequested=false;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CRazberry::Do_Work, this)));
	return (m_thread!=NULL);
}

bool CRazberry::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	return true;
}

void CRazberry::Do_Work()
{
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		if (m_bInitState)
		{
			if (GetInitialDevices())
			{
				m_bInitState=false;
				sOnConnected(this);
			}
		}
		else
			GetUpdates();
	}
}

const std::string CRazberry::GetControllerURL()
{
	std::stringstream sUrl;
	if (m_username=="")
		sUrl << "http://" << m_ipaddress << ":" << m_port << "/ZWaveAPI/Data/" << m_updateTime;
	else
		sUrl << "http://"  << m_username << ":" << m_password << "@" << m_ipaddress << ":" << m_port << "/ZWaveAPI/Data/" << m_updateTime;
	return sUrl.str();
}

const std::string CRazberry::GetRunURL(const std::string cmd)
{
	std::stringstream sUrl;
	if (m_username=="")
		sUrl << "http://" << m_ipaddress << ":" << m_port << "/ZWaveAPI/Run/" << cmd;
	else
		sUrl << "http://"  << m_username << ":" << m_password << "@" << m_ipaddress << ":" << m_port << "/ZWaveAPI/Run/" << cmd;
	return sUrl.str();
}

bool CRazberry::GetInitialDevices()
{
	std::string sResult;
#ifndef DEBUG_ZWAVE_INT	
	std::string szURL=GetControllerURL();

	bool bret;
	bret=HTTPClient::GET(szURL,sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR,"Error getting Razberry data!");
		return 0;
	}
#else
	sResult=readInputTestFile("test.json");
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"Razberry: Invalid data received!");
		return 0;
	}

	Json::Value jval;
	jval=root["controller"];
	if (jval.empty()==true)
		return 0;
	m_controllerID=jval["data"]["nodeId"]["value"].asInt();

	for (Json::Value::iterator itt=root.begin(); itt!=root.end(); ++itt)
	{
		const std::string kName=itt.key().asString();
		if (kName=="devices")
		{
			parseDevices(*itt);
		}
		else if (kName=="updateTime")
		{
			std::string supdateTime=(*itt).asString();
			m_updateTime=(time_t)atol(supdateTime.c_str());
		}
	}

	return true;
}

bool CRazberry::GetUpdates()
{
	std::string sResult;
#ifndef	DEBUG_ZWAVE_INT
	std::string szURL=GetControllerURL();
	bool bret;
	bret=HTTPClient::GET(szURL,sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR,"Razberry: Error getting update data!");
		return 0;
	}
#else
	sResult=readInputTestFile("update.json");
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"Razberry: Invalid data received!");
		return 0;
	}

	for (Json::Value::iterator itt=root.begin(); itt!=root.end(); ++itt)
	{
		std::string kName=itt.key().asString();
		const Json::Value obj=(*itt);

		if (kName=="updateTime")
		{
			std::string supdateTime=obj.asString();
			m_updateTime=(time_t)atol(supdateTime.c_str());
		}
		else if (kName=="devices")
		{
			parseDevices(obj);
		}
		else
		{
			std::vector<std::string> results;
			StringSplit(kName,".",results);

			if (results.size()>1)
			{
				UpdateDevice(kName,obj);
			}
		}
	}

	return true;
}

void CRazberry::parseDevices(const Json::Value devroot)
{
	for (Json::Value::iterator itt=devroot.begin(); itt!=devroot.end(); ++itt)
	{
		const std::string devID=itt.key().asString();

		unsigned long nodeID=atol(devID.c_str());
		if ((nodeID==255)||(nodeID==m_controllerID))
			continue; //skip ourself

		const Json::Value node=(*itt);

		_tZWaveDevice _device;
		_device.nodeID=nodeID;
		// Device status and battery
		_device.basicType =		node["data"]["basicType"]["value"].asInt();
		_device.genericType =	node["data"]["genericType"]["value"].asInt();
		_device.specificType =	node["data"]["specificType"]["value"].asInt();
		_device.isListening =	node["data"]["isListening"]["value"].asBool();
		_device.sensor250=		node["data"]["sensor250"]["value"].asBool();
		_device.sensor1000=		node["data"]["sensor1000"]["value"].asBool();
		_device.isFLiRS =		!_device.isListening && (_device.sensor250 || _device.sensor1000);
		_device.hasWakeup =		(node["instances"]["0"]["commandClasses"]["132"].empty()==false);
		_device.hasBattery =	(node["instances"]["0"]["commandClasses"]["128"].empty()==false);

		const Json::Value nodeInstances=node["instances"];
		// For all instances
		bool haveMultipleInstance=(nodeInstances.size()>1);
		for (Json::Value::iterator ittInstance=nodeInstances.begin(); ittInstance!=nodeInstances.end(); ++ittInstance)
		{
			_device.commandClassID=0;
			_device.scaleID=-1;

			const std::string sID=ittInstance.key().asString();
			_device.instanceID=atoi(sID.c_str());
			if ((_device.instanceID==0)&&(haveMultipleInstance))
				continue;// We skip instance 0 if there are more, since it should be mapped to other instances or their superposition

			const Json::Value instance=(*ittInstance);

			if (_device.hasBattery)
			{
				_device.batValue=instance["commandClasses"]["128"]["data"]["last"]["value"].asInt();
			}

			// Switches
			// We choose SwitchMultilevel first, if not available, SwhichBinary is chosen
			if (instance["commandClasses"]["38"].empty()==false)
			{
				_device.commandClassID=38;
				_device.devType= ZDTYPE_SWITCHDIMMER;
				_device.intvalue=instance["commandClasses"]["38"]["data"]["level"]["value"].asInt();
				InsertOrUpdateDevice(_device,true);
			}
			else if (instance["commandClasses"]["37"].empty()==false)
			{
				_device.commandClassID=37;
				_device.devType= ZDTYPE_SWITCHNORMAL;
				_device.intvalue=instance["commandClasses"]["37"]["data"]["level"]["value"].asInt();
				InsertOrUpdateDevice(_device,true);
			}

			// Add SensorMultilevel
			if (instance["commandClasses"]["48"].empty()==false)
			{
				_device.commandClassID=48; //(binary switch, for example motion detector(PIR)
				_device.devType= ZDTYPE_SWITCHNORMAL;
				_device.intvalue=instance["commandClasses"]["48"]["data"]["level"]["value"].asInt();
				InsertOrUpdateDevice(_device,true);
			}

			if (instance["commandClasses"]["49"].empty()==false)
			{
				_device.commandClassID=49;
				_device.scaleMultiply=1;
				const Json::Value inVal=instance["commandClasses"]["49"]["data"];
				for (Json::Value::iterator itt2=inVal.begin(); itt2!=inVal.end(); ++itt2)
				{
					const std::string sKey=itt2.key().asString();
					if (!isInt(sKey))
						continue; //not a scale
					_device.scaleID=atoi(sKey.c_str());
					std::string sensorTypeString = (*itt2)["sensorTypeString"]["value"].asString();
					if (sensorTypeString=="Power")
					{
						_device.floatValue=(*itt2)["val"]["value"].asFloat();
						if (_device.scaleID == 4)
						{
							_device.commandClassID=49;
							_device.devType = ZDTYPE_SENSOR_POWER;
							InsertOrUpdateDevice(_device,false);
						}
					}
					else if (sensorTypeString=="Temperature")
					{
						_device.floatValue=(*itt2)["val"]["value"].asFloat();
						_device.commandClassID=49;
						_device.devType = ZDTYPE_SENSOR_TEMPERATURE;
						InsertOrUpdateDevice(_device,false);
					}
					else if (sensorTypeString=="Humidity")
					{
						_device.intvalue=(*itt2)["val"]["value"].asInt();
						_device.commandClassID=49;
						_device.devType = ZDTYPE_SENSOR_HUMIDITY;
						InsertOrUpdateDevice(_device,false);
					}
					else if (sensorTypeString=="Luminiscence")
					{
						_device.floatValue=(*itt2)["val"]["value"].asFloat();
						_device.commandClassID=49;
						_device.devType = ZDTYPE_SENSOR_LIGHT;
						InsertOrUpdateDevice(_device,false);
					}
				}
				//InsertOrUpdateDevice(_device);
			}

			// Meters which are supposed to be sensors (measurable)
			if (instance["commandClasses"]["50"].empty()==false)
			{
				const Json::Value inVal=instance["commandClasses"]["50"]["data"];
				for (Json::Value::iterator itt2=inVal.begin(); itt2!=inVal.end(); ++itt2)
				{
					const std::string sKey=itt2.key().asString();
					if (!isInt(sKey))
						continue; //not a scale
					_device.scaleID=atoi(sKey.c_str());
					int sensorType=(*itt2)["sensorType"]["value"].asInt();
					_device.floatValue=(*itt2)["val"]["value"].asFloat();
					std::string scaleString = (*itt2)["scaleString"]["value"].asString();
					if ((_device.scaleID == 0 || _device.scaleID == 2 || _device.scaleID == 4 || _device.scaleID == 6) && (sensorType == 1))
					{
						_device.commandClassID=50;
						_device.devType = ZDTYPE_SENSOR_POWER;
						_device.scaleMultiply=1;
						if (scaleString=="kWh")
							_device.scaleMultiply=1000;

						InsertOrUpdateDevice(_device,false);
					}
				}
			}

			// Meters (true meter values)
			if (instance["commandClasses"]["50"].empty()==false)
			{
				const Json::Value inVal=instance["commandClasses"]["50"]["data"];
				for (Json::Value::iterator itt2=inVal.begin(); itt2!=inVal.end(); ++itt2)
				{
					const std::string sKey=itt2.key().asString();
					if (!isInt(sKey))
						continue; //not a scale
					_device.scaleID=atoi(sKey.c_str());
					int sensorType=(*itt2)["sensorType"]["value"].asInt();
					_device.floatValue=(*itt2)["val"]["value"].asFloat();
					std::string scaleString = (*itt2)["scaleString"]["value"].asString();
					if ((_device.scaleID == 0 || _device.scaleID == 2 || _device.scaleID == 4 || _device.scaleID == 6) && (sensorType == 1))
						continue; // we don't want to have measurable here (W, V, PowerFactor)
					_device.commandClassID=50;
					_device.scaleMultiply=1;
					if (scaleString=="kWh")
						_device.scaleMultiply=1000;
					_device.devType = ZDTYPE_SENSOR_POWER;
					InsertOrUpdateDevice(_device,false);
				}
			}

			// Thermostat Set Point
			if (instance["commandClasses"]["67"].empty()==false)
			{
				const Json::Value inVal=instance["commandClasses"]["67"]["data"];
				for (Json::Value::iterator itt2=inVal.begin(); itt2!=inVal.end(); ++itt2)
				{
					const std::string sKey=itt2.key().asString();
					if (!isInt(sKey))
						continue; //not a scale
					_device.floatValue=(*itt2)["val"]["value"].asFloat();
					_device.commandClassID=67;
					_device.scaleMultiply=1;
					_device.devType = ZDTYPE_SENSOR_SETPOINT;
					InsertOrUpdateDevice(_device,false);
				}
			}

			//COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE 70
		}
	}
}

void CRazberry::InsertOrUpdateDevice(_tZWaveDevice device, const bool bSend2Domoticz)
{
	std::stringstream sstr;
	sstr << device.nodeID << ".instances." << device.instanceID << ".commandClasses." << device.commandClassID << ".data";
	if (device.scaleID!=-1)
	{
		sstr << "." << device.scaleID;
	}
	device.string_id=sstr.str();

	bool bNewDevice=(m_devices.find(device.string_id)==m_devices.end());
	
	device.lastreceived=time(NULL);
#ifdef _DEBUG
	if (bNewDevice)
	{
		std::cout << "New device: " << device.string_id << std::endl;
	}
	else
	{
		std::cout << "Update device: " << device.string_id << std::endl;
	}
#endif
	//insert or update device in internal record
	device.sequence_number=1;
	m_devices[device.string_id]=device;
	//do not send (for now), as this will trigger event/notifications
/*
	if (bSend2Domoticz)
	{
		SendDevice2Domoticz(&device);
	}
*/
}

void CRazberry::UpdateDevice(const std::string path, const Json::Value obj)
{
	_tZWaveDevice *pDevice=NULL;

	//Possible fix for Everspring ST814. maybe others, my multi sensor is coming soon to find out!
	if (path.find("instances.0.commandClasses.49.data.")!=std::string::npos)
	{
		std::vector<std::string> results;
		StringSplit(path,".",results);
		//Find device by data id
		if (results.size()==8)
		{
			int cmdID=atoi(results[5].c_str());
			if (cmdID==49)
			{
				int devID=atoi(results[1].c_str());
				int scaleID=atoi(results[7].c_str());
				pDevice=FindDevice(devID,scaleID);
			}
		}
	}
	if (pDevice==NULL)
	{
		std::map<std::string,_tZWaveDevice>::iterator itt;
		for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
		{
			std::string::size_type loc = path.find(itt->second.string_id,0);
			if (loc!=std::string::npos)
			{
				pDevice=&itt->second;
				break;
			}
		}
	}

	if (pDevice==NULL)
	{
		return; //don't know you
	}

	time_t atime=time(NULL);
//	if (atime-pDevice->lastreceived<2)
	//	return; //to soon
#ifdef _DEBUG
	std::cout << asctime(localtime(&atime));
	std::cout << "Razberry: Update device: " << pDevice->string_id << std::endl;
#endif

	switch (pDevice->devType)
	{
	case ZDTYPE_SWITCHNORMAL:
	case ZDTYPE_SWITCHDIMMER:
		//switch
		pDevice->intvalue=obj["value"].asInt();
		break;
	case ZDTYPE_SENSOR_POWER:
		//meters
		pDevice->floatValue=obj["val"]["value"].asFloat()*pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_TEMPERATURE:
		//meters
		pDevice->floatValue=obj["val"]["value"].asFloat();
		break;
	case ZDTYPE_SENSOR_HUMIDITY:
		//switch
		pDevice->intvalue=obj["val"]["value"].asInt();
		break;
	case ZDTYPE_SENSOR_LIGHT:
		//switch
		pDevice->floatValue=obj["val"]["value"].asFloat();
		break;
	case ZDTYPE_SENSOR_SETPOINT:
		//meters
		if (obj["val"]["value"].empty()==false)
		{
			pDevice->floatValue=obj["val"]["value"].asFloat();
		}
		else if (obj["value"].empty()==false)
		{
			pDevice->floatValue=obj["value"].asFloat();
		}
		break;
	}

	pDevice->lastreceived=atime;
	pDevice->sequence_number+=1;
	if (pDevice->sequence_number==0)
		pDevice->sequence_number=1;
	SendDevice2Domoticz(pDevice);
}

void CRazberry::SendDevice2Domoticz(const _tZWaveDevice *pDevice)
{
	unsigned char ID1=0;
	unsigned char ID2=0;
	unsigned char ID3=0;
	unsigned char ID4=0;

	//make device ID
	ID1=0;
	ID2=(unsigned char)((pDevice->nodeID&0xFF00)>>8);
	ID3=(unsigned char)pDevice->nodeID&0xFF;
	ID4=pDevice->instanceID;

	char szID[10];
	sprintf(szID,"%X%02X%02X%02X", ID1, ID2, ID3, ID4);


	if ((pDevice->devType==ZDTYPE_SWITCHNORMAL)||(pDevice->devType==ZDTYPE_SWITCHDIMMER))
	{
		//Send as Lighting 2

		tRBUF lcmd;
		memset(&lcmd,0,sizeof(RBUF));
		lcmd.LIGHTING2.packetlength=sizeof(lcmd.LIGHTING2)-1;
		lcmd.LIGHTING2.packettype=pTypeLighting2;
		lcmd.LIGHTING2.subtype=sTypeAC;
		lcmd.LIGHTING2.seqnbr=pDevice->sequence_number;
		lcmd.LIGHTING2.id1=ID1;
		lcmd.LIGHTING2.id2=ID2;
		lcmd.LIGHTING2.id3=ID3;
		lcmd.LIGHTING2.id4=ID4;
		lcmd.LIGHTING2.unitcode=1;
		int level=15;
		if (pDevice->devType==ZDTYPE_SWITCHNORMAL)
		{
			//simple on/off device
			if (pDevice->intvalue==0)
			{
				level=0;
				lcmd.LIGHTING2.cmnd=light2_sOff;
			}
			else
			{
				level=15;
				lcmd.LIGHTING2.cmnd=light2_sOn;
			}
		}
		else
		{
			//dimmer able device
			if (pDevice->intvalue==0)
				level=0;
			if (pDevice->intvalue==255)
				level=15;
			else
			{
				float flevel=(15.0f/100.0f)*float(pDevice->intvalue);
				level=round(flevel);
				if (level>15)
					level=15;
			}
			if (level==0)
				lcmd.LIGHTING2.cmnd=light2_sOff;
			else if (level==15)
				lcmd.LIGHTING2.cmnd=light2_sOn;
			else
				lcmd.LIGHTING2.cmnd=light2_sSetLevel;
		}
		lcmd.LIGHTING2.level=level;
		lcmd.LIGHTING2.filler=0;
		lcmd.LIGHTING2.rssi=7;
		sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);//decode message
		return;
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_POWER)
	{
		_tUsageMeter umeter;
		umeter.id1=ID1;
		umeter.id2=ID2;
		umeter.id3=ID3;
		umeter.id4=ID4;
		umeter.dunit=pDevice->scaleID;
		umeter.fusage=pDevice->floatValue;
		sDecodeRXMessage(this, (const unsigned char *)&umeter);//decode message
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_TEMPERATURE)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.TEMP_HUM.battery_level=9;
		tsen.TEMP_HUM.rssi=6;
		tsen.TEMP.id1=ID3;
		tsen.TEMP.id2=ID4;

		const _tZWaveDevice *pHumDevice=FindDevice(pDevice->nodeID,-1,ZDTYPE_SENSOR_HUMIDITY);
		if (pHumDevice)
		{
			tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
			tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
			tsen.TEMP_HUM.subtype=sTypeTH5;

			tsen.TEMP_HUM.tempsign=(pDevice->floatValue>=0)?0:1;
			int at10=round(abs(pDevice->floatValue*10.0f));
			tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
			at10-=(tsen.TEMP_HUM.temperatureh*256);
			tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
			tsen.TEMP_HUM.humidity=(BYTE)pHumDevice->intvalue;
			tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);
		}
		else
		{
			tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
			tsen.TEMP.packettype=pTypeTEMP;
			tsen.TEMP.subtype=sTypeTEMP10;
			tsen.TEMP.tempsign=(pDevice->floatValue>=0)?0:1;
			int at10=round(abs(pDevice->floatValue*10.0f));
			tsen.TEMP.temperatureh=(BYTE)(at10/256);
			at10-=(tsen.TEMP.temperatureh*256);
			tsen.TEMP.temperaturel=(BYTE)(at10);
		}
		sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_HUMIDITY)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.TEMP_HUM.battery_level=9;
		tsen.TEMP_HUM.rssi=6;
		tsen.TEMP.id1=ID3;
		tsen.TEMP.id2=ID4;

		const _tZWaveDevice *pTempDevice=FindDevice(pDevice->nodeID,-1,ZDTYPE_SENSOR_TEMPERATURE);
		if (pTempDevice)
		{
			tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
			tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
			tsen.TEMP_HUM.subtype=sTypeTH5;
			ID4=pTempDevice->instanceID;
			tsen.TEMP.id2=ID4;

			tsen.TEMP_HUM.tempsign=(pTempDevice->floatValue>=0)?0:1;
			int at10=round(abs(pTempDevice->floatValue*10.0f));
			tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
			at10-=(tsen.TEMP_HUM.temperatureh*256);
			tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
			tsen.TEMP_HUM.humidity=(BYTE)pDevice->intvalue;
			tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);
		}
		else
		{
			memset(&tsen,0,sizeof(RBUF));
			tsen.HUM.packetlength=sizeof(tsen.HUM)-1;
			tsen.HUM.packettype=pTypeHUM;
			tsen.HUM.subtype=sTypeHUM2;
			tsen.HUM.humidity=(BYTE)pDevice->intvalue;
			tsen.HUM.humidity_status=Get_Humidity_Level(tsen.HUM.humidity);
		}
		sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_LIGHT)
	{
		_tLightMeter lmeter;
		lmeter.id1=ID1;
		lmeter.id2=ID2;
		lmeter.id3=ID3;
		lmeter.id4=ID4;
		lmeter.dunit=pDevice->scaleID;
		lmeter.fLux=pDevice->floatValue;
		sDecodeRXMessage(this, (const unsigned char *)&lmeter);
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_SETPOINT)
	{
		_tThermostat tmeter;
		tmeter.subtype=sTypeThermSetpoint;
		tmeter.id1=ID1;
		tmeter.id2=ID2;
		tmeter.id3=ID3;
		tmeter.id4=ID4;
		tmeter.dunit=1;
		tmeter.temp=pDevice->floatValue;
		sDecodeRXMessage(this, (const unsigned char *)&tmeter);
	}
}

CRazberry::_tZWaveDevice* CRazberry::FindDevice(int nodeID, int instanceID, _eZWaveDeviceType devType)
{
	std::map<std::string,_tZWaveDevice>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		if (
			(itt->second.nodeID==nodeID)&&
			((itt->second.instanceID==instanceID)||(instanceID==-1))&&
			(itt->second.devType==devType)
			)
			return &itt->second;
	}
	return NULL;
}

CRazberry::_tZWaveDevice* CRazberry::FindDevice(int nodeID, int scaleID)
{
	std::map<std::string,_tZWaveDevice>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		if (
			(itt->second.nodeID==nodeID)&&
			(itt->second.scaleID==scaleID)
			)
			return &itt->second;
	}
	return NULL;
}

void CRazberry::WriteToHardware(const char *pdata, const unsigned char length)
{
	unsigned char ID1=0;
	unsigned char ID2=0;
	unsigned char ID3=0;
	unsigned char ID4=0;

	const _tZWaveDevice* pDevice=NULL;

	tRBUF *pSen=(tRBUF*)pdata;

	unsigned char packettype=pSen->ICMND.packettype;
	unsigned char subtype=pSen->ICMND.subtype;

	if (packettype==pTypeLighting2)
	{
		//light command

		//find device

		int nodeID=(pSen->LIGHTING2.id2<<8)|pSen->LIGHTING2.id3;
		int instanceID=pSen->LIGHTING2.id4;

		int svalue=0;

		//find normal
		pDevice=FindDevice(nodeID,instanceID,ZDTYPE_SWITCHNORMAL);
		if (pDevice)
		{
			if (pSen->LIGHTING2.cmnd==light2_sOff)
				svalue=0;
			else
				svalue=255;
		}
		else {
			//find dimmer
			pDevice=FindDevice(nodeID,instanceID,ZDTYPE_SWITCHDIMMER);
			if (!pDevice)
				return;//ehh dont know you!

			if (pSen->LIGHTING2.cmnd==light2_sOff)
				svalue=0;
			if (pSen->LIGHTING2.cmnd==light2_sOn)
				svalue=255;
			else
			{
				float fvalue=(100.0f/15.0f)*float(pSen->LIGHTING2.level);
				if (fvalue>100.0f)
					fvalue=100.0f;
				svalue=round(fvalue);
			}
		}
		//Send command
		std::stringstream sstr;
		sstr << "devices[" << nodeID << "].instances[" << instanceID << "].commandClasses[" << pDevice->commandClassID << "].Set(" << svalue << ")";
		RunCMD(sstr.str());
	}
	else if ((packettype==pTypeThermostat)&&(subtype==sTypeThermSetpoint))
	{
		//Set Point
		_tThermostat *pMeter=(_tThermostat *)pSen;

		int nodeID=(pMeter->id2<<8)|pMeter->id3;
		int instanceID=pMeter->id4;

		//find normal
		pDevice=FindDevice(nodeID,instanceID,ZDTYPE_SENSOR_SETPOINT);
		if (pDevice)
		{
			std::stringstream sstr;
			sstr << "devices[" << nodeID << "].instances[" << instanceID << "].commandClasses[" << pDevice->commandClassID << "].Set(1," << pMeter->temp << ",null)";
			RunCMD(sstr.str());
		}
	}
}

void CRazberry::RunCMD(const std::string cmd)
{
	std::string szURL=GetRunURL(cmd);
	bool bret;
	std::string sResult;
	bret=HTTPClient::GET(szURL,sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR,"Razberry: Error sending command to controller!");
	}
}

