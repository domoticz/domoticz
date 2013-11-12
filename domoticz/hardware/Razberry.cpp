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

#include "../json/json.h"
#include "../main/localtime_r.h"

#pragma warning(disable: 4996)

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
	#define DEBUG_ZWAVE_INT
#endif

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

CRazberry::CRazberry(const int ID, const std::string &ipaddress, const int port, const std::string &username, const std::string &password)
{
	m_HwdID=ID;
	m_ipaddress=ipaddress;
	m_port=port;
	m_username=username;
	m_password=password;
	m_controllerID=0;
}


CRazberry::~CRazberry(void)
{
}

void CRazberry::StopHardwareIntern()
{

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

const std::string CRazberry::GetRunURL(const std::string &cmd)
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
				if (kName.find("lastReceived")==std::string::npos)
					UpdateDevice(kName,obj);
			}
		}
	}

	return true;
}

void CRazberry::parseDevices(const Json::Value &devroot)
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
				bool bFoundSensor=false;
				if (instance["commandClasses"]["48"]["data"]["level"].empty()==false)
				{
					_device.intvalue=instance["commandClasses"]["48"]["data"]["level"]["value"].asInt();
					bFoundSensor=true;
				}
				else
				{
					if (instance["commandClasses"]["48"]["data"]["0"].empty()==false)
					{
						_device.intvalue=instance["commandClasses"]["48"]["data"]["0"]["level"]["value"].asInt();
						bFoundSensor=true;
					}
					else if (instance["commandClasses"]["48"]["data"]["1"].empty()==false)
					{
						std::string vstring=instance["commandClasses"]["48"]["data"]["1"]["level"]["value"].asString();
						if (vstring=="true")
							_device.intvalue=255;
						else if (vstring=="false")
							_device.intvalue=0;
						else
							_device.intvalue=atoi(vstring.c_str());
						bFoundSensor=true;
					}
				}
				if (bFoundSensor)
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
						_device.scaleMultiply=1;
						if (scaleString=="kWh")
						{
							_device.scaleMultiply=1000;
							_device.devType = ZDTYPE_SENSOR_POWERENERGYMETER;
						}
						else
						{
							_device.devType = ZDTYPE_SENSOR_POWER;
						}

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
					{
						_device.scaleMultiply=1000;
						_device.devType = ZDTYPE_SENSOR_POWERENERGYMETER;
					}
					else
					{
						_device.devType = ZDTYPE_SENSOR_POWER;
					}
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
			else if (instance["commandClasses"]["156"].empty()==false)
			{
				const Json::Value inVal=instance["commandClasses"]["156"]["data"];
				for (Json::Value::iterator itt2=inVal.begin(); itt2!=inVal.end(); ++itt2)
				{
					const std::string sKey=itt2.key().asString();
					if (!isInt(sKey))
						continue; //not a scale
					_device.intvalue=(*itt2)["sensorSate"]["value"].asInt();
					_device.commandClassID=156;
					_device.devType = ZDTYPE_SWITCHNORMAL;
					InsertOrUpdateDevice(_device,false);
				}
			}

			//COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE 70
		}
	}
}

void CRazberry::UpdateDevice(const std::string &path, const Json::Value &obj)
{
	_tZWaveDevice *pDevice=NULL;

	if (
		(path.find("srcId")!=std::string::npos)||
		(path.find("sensorTime")!=std::string::npos)
		)
		return;

	if (path.find("instances.0.commandClasses.128.data.last")!=std::string::npos)
	{
		if (obj["value"].empty()==false)
		{
			int batValue=obj["value"].asInt();
			std::vector<std::string> results;
			StringSplit(path,".",results);
			int devID=atoi(results[1].c_str());
			UpdateDeviceBatteryStatus(devID,batValue);
		}
	}
	else if (path.find("instances.0.commandClasses.49.data.")!=std::string::npos)
	{
		//Possible fix for Everspring ST814. maybe others, my multi sensor is coming soon to find out!
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
		//Special Case for Controller received light commands
		size_t iPos= path.find("commandClasses.32.data.level");
		if (iPos==std::string::npos)
			return;
		std::string tmpStr;
		//create this device
		_tZWaveDevice _device;

		//Get device node ID
		size_t pPos=path.find(".");
		if (pPos==std::string::npos)
			return;
		tmpStr=path.substr(pPos+1);
		pPos=pPos=tmpStr.find(".");
		if (pPos==std::string::npos)
			return;
		std::string sNodeID=tmpStr.substr(0,pPos);
		_device.nodeID=atoi(sNodeID.c_str());

		//Find instance ID
		pPos=path.find("instances.");
		if (pPos==std::string::npos)
			return;
		tmpStr=path.substr(pPos+sizeof("instances.")-1);
		pPos=pPos=tmpStr.find(".");
		if (pPos==std::string::npos)
			return;
		std::string sInstanceID=tmpStr.substr(0,pPos);
		_device.instanceID=atoi(sInstanceID.c_str());
		

		// Device status and battery
		_device.basicType =		1;
		_device.genericType =	1;
		_device.specificType =	1;
		_device.isListening =	false;
		_device.sensor250=		false;
		_device.sensor1000=		false;
		_device.isFLiRS =		!_device.isListening && (_device.sensor250 || _device.sensor1000);
		_device.hasWakeup =		false;
		_device.hasBattery =	false;
		_device.scaleID=-1;

		_device.commandClassID=32;
		_device.devType= ZDTYPE_SWITCHNORMAL;
		_device.intvalue=obj["value"].asInt();
		InsertOrUpdateDevice(_device,true);

		//find device again
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
		if (pDevice==NULL)
			return;//ehuh?
	}

	time_t atime=mytime(NULL);
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
		{
			//switch
			std::string vstring="";
			if (obj["value"].empty()==false)
				vstring=obj["value"].asString();
			else if (obj["level"].empty()==false)
			{
				if (obj["level"]["value"].empty()==false)
					vstring=obj["level"]["value"].asString();
			}

			if (vstring=="true")
				pDevice->intvalue=255;
			else if (vstring=="false")
				pDevice->intvalue=0;
			else
				pDevice->intvalue=atoi(vstring.c_str());
		}
		break;
	case ZDTYPE_SENSOR_POWER:
		//meters
		pDevice->floatValue=obj["val"]["value"].asFloat()*pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_POWERENERGYMETER:
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

void CRazberry::SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value)
{
	//Send command
	std::stringstream sstr;
	sstr << "devices[" << nodeID << "].instances[" << instanceID << "].commandClasses[" << commandClass << "].Set(" << value << ")";
	RunCMD(sstr.str());
}

void CRazberry::SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value)
{
	std::stringstream sstr;
	sstr << "devices[" << nodeID << "].instances[" << instanceID << "].commandClasses[" << commandClass << "].Set(1," << value << ",null)";
	RunCMD(sstr.str());
}

void CRazberry::RunCMD(const std::string &cmd)
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

