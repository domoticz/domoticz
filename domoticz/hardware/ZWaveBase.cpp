#include "stdafx.h"
#include "ZWaveBase.h"

#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <vector>
#include <ctype.h>

#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"

#include "../json/json.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"

#define CONTROLLER_COMMAND_TIMEOUT 20

#pragma warning(disable: 4996)

#define round(a) ( int ) ( a + .5 )

ZWaveBase::ZWaveBase()
{
	m_LastIncludedNode=0;
	m_bControllerCommandInProgress=false;
	m_updateTime=0;
}


ZWaveBase::~ZWaveBase(void)
{
}

bool ZWaveBase::StartHardware()
{
	m_bInitState=true;
	m_stoprequested=false;
	m_updateTime=0;
	m_LastIncludedNode=0;
	m_bControllerCommandInProgress=false;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&ZWaveBase::Do_Work, this)));
	return (m_thread!=NULL);
}

bool ZWaveBase::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		if (m_thread!=NULL)
			m_thread->join();
	}
	m_bIsStarted=false;
	return true;
}

void ZWaveBase::Do_Work()
{
#ifdef WIN32
	//prevent OpenZWave locale from taking over
	_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif
	while (!m_stoprequested)
	{
		sleep_milliseconds(500);

		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);


		if (ltime.tm_sec % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}

		if (m_stoprequested)
			return;
		if (m_bInitState)
		{
			if (GetInitialDevices())
			{
				m_bInitState=false;
				sOnConnected(this);
			}
		}
		else
		{
			GetUpdates();
			if (m_bControllerCommandInProgress==true)
			{
				time_t atime=mytime(NULL);
				time_t tdiff=atime-m_ControllerCommandStartTime;
				if (tdiff>=CONTROLLER_COMMAND_TIMEOUT)
				{
					_log.Log(LOG_ERROR,"ZWave: Stopping Controller command (Timeout!)");
					CancelControllerCommand();
				}
			}
		}
	}
}


std::string ZWaveBase::GenerateDeviceStringID(const _tZWaveDevice *pDevice)
{
	std::stringstream sstr;
	sstr << pDevice->nodeID << ".instances." << pDevice->instanceID << ".commandClasses." << pDevice->commandClassID << ".data";
	if (pDevice->scaleID!=-1)
	{
		sstr << "." << pDevice->scaleID;
	}
	return sstr.str();
}

void ZWaveBase::InsertDevice(_tZWaveDevice device)
{
	device.string_id=GenerateDeviceStringID(&device);

	bool bNewDevice=(m_devices.find(device.string_id)==m_devices.end());
	
	device.lastreceived=mytime(NULL);
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

	SendSwitchIfNotExists(&device);
}

void ZWaveBase::UpdateDeviceBatteryStatus(const int nodeID, const int value)
{
	std::map<std::string,_tZWaveDevice>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		if (itt->second.nodeID==nodeID)
		{
			itt->second.batValue=value;
			itt->second.hasBattery=true;//we got an update, so it should have a battery then...
		}
	}
}

void ZWaveBase::SendSwitchIfNotExists(const _tZWaveDevice *pDevice)
{
	if (
		(pDevice->devType!=ZDTYPE_SWITCH_NORMAL)&&
		(pDevice->devType != ZDTYPE_SWITCH_DIMMER) &&
		(pDevice->devType != ZDTYPE_SWITCH_FGRGBWM441)
		)
		return; //only for switches

	if (pDevice->devType == ZDTYPE_SWITCH_FGRGBWM441)
	{
		unsigned char ID1 = 0;
		unsigned char ID2 = 0;
		unsigned char ID3 = 0;
		unsigned char ID4 = 0;

		//make device ID
		ID1 = 0;
		ID2 = (unsigned char)((pDevice->nodeID & 0xFF00) >> 8);
		ID3 = (unsigned char)pDevice->nodeID & 0xFF;
		ID4 = pDevice->instanceID;

		//To fix all problems it should be
		//ID1 = (unsigned char)((pDevice->nodeID & 0xFF00) >> 8);
		//ID2 = (unsigned char)pDevice->nodeID & 0xFF;
		//ID3 = pDevice->instanceID;
		//ID4 = pDevice->indexID;
		//but current users gets new devices in this case

		unsigned long lID = (ID1 << 24) + (ID2 << 16) + (ID3 << 8) + ID4;

		char szID[10];
		sprintf(szID, "%08x", (unsigned int)lID);
		std::string ID = szID;
		unsigned char unitcode = 1;

		//Check if we already exist
		std::stringstream szQuery;
		std::vector<std::vector<std::string> > result;
		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==" << int(unitcode) << ") AND (Type==" << pTypeLimitlessLights << ") AND (SubType==" << sTypeLimitlessRGBW << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
		if (result.size() > 0)
			return; //Already in the system

		//Send as LimitlessLight
		_tLimitlessLights lcmd;
		lcmd.id = lID;
		lcmd.command = Limitless_LedOff;
		lcmd.value = 0;
		sDecodeRXMessage(this, (const unsigned char *)&lcmd);

		//Set Name
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << pDevice->label << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
	}
	else
	{
		unsigned char ID1 = 0;
		unsigned char ID2 = 0;
		unsigned char ID3 = 0;
		unsigned char ID4 = 0;

		//make device ID
		ID1 = 0;
		ID2 = (unsigned char)((pDevice->nodeID & 0xFF00) >> 8);
		ID3 = (unsigned char)pDevice->nodeID & 0xFF;
		ID4 = pDevice->instanceID;

		//To fix all problems it should be
		//ID1 = (unsigned char)((pDevice->nodeID & 0xFF00) >> 8);
		//ID2 = (unsigned char)pDevice->nodeID & 0xFF;
		//ID3 = pDevice->instanceID;
		//ID4 = pDevice->indexID;
		//but current users gets new devices in this case

		char szID[10];
		sprintf(szID, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
		std::string ID = szID;
		unsigned char unitcode = 1;

		//Check if we already exist
		std::stringstream szQuery;
		std::vector<std::vector<std::string> > result;
		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==" << int(unitcode) << ") AND (Type==" << pTypeLighting2 << ") AND (SubType==" << sTypeAC << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
		if (result.size() > 0)
			return; //Already in the system

		//Send as Lighting 2

		tRBUF lcmd;
		memset(&lcmd, 0, sizeof(RBUF));
		lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
		lcmd.LIGHTING2.packettype = pTypeLighting2;
		lcmd.LIGHTING2.subtype = sTypeAC;
		lcmd.LIGHTING2.seqnbr = pDevice->sequence_number;
		lcmd.LIGHTING2.id1 = ID1;
		lcmd.LIGHTING2.id2 = ID2;
		lcmd.LIGHTING2.id3 = ID3;
		lcmd.LIGHTING2.id4 = ID4;
		lcmd.LIGHTING2.unitcode = unitcode;
		int level = 15;
		if (pDevice->devType == ZDTYPE_SWITCH_NORMAL)
		{
			//simple on/off device
			if (pDevice->intvalue == 0)
			{
				level = 0;
				lcmd.LIGHTING2.cmnd = light2_sOff;
			}
			else
			{
				level = 15;
				lcmd.LIGHTING2.cmnd = light2_sOn;
			}
		}
		else
		{
			//dimmer able device
			if (pDevice->intvalue == 0)
				level = 0;
			if (pDevice->intvalue == 255)
				level = 15;
			else
			{
				float flevel = (15.0f / 100.0f)*float(pDevice->intvalue);
				level = round(flevel);
				if (level > 15)
					level = 15;
			}
			if (level == 0)
				lcmd.LIGHTING2.cmnd = light2_sOff;
			else if (level == 15)
				lcmd.LIGHTING2.cmnd = light2_sOn;
			else
				lcmd.LIGHTING2.cmnd = light2_sSetLevel;
		}
		lcmd.LIGHTING2.level = level;
		lcmd.LIGHTING2.filler = 0;
		lcmd.LIGHTING2.rssi = 12;

		sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);

		//Set Name
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << pDevice->label << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
	}
}

unsigned char ZWaveBase::Convert_Battery_To_PercInt(const unsigned char level)
{
	int ret=(level/10)-1;
	if (ret<0)
		ret = 0;
	return (unsigned char)ret;
}

void ZWaveBase::SendDevice2Domoticz(const _tZWaveDevice *pDevice)
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


	if ((pDevice->devType==ZDTYPE_SWITCH_NORMAL)||(pDevice->devType==ZDTYPE_SWITCH_DIMMER))
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
		if (pDevice->devType==ZDTYPE_SWITCH_NORMAL)
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
		lcmd.LIGHTING2.rssi=12;
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
	else if (pDevice->devType==ZDTYPE_SENSOR_VOLTAGE)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype=sTypeVoltage;
		gDevice.id=ID4;
		gDevice.floatval1=pDevice->floatValue;
		gDevice.intval1=(int)(ID1<<24)|(ID2<<16)|(ID3<<8)|ID4;
		sDecodeRXMessage(this, (const unsigned char *)&gDevice);
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_PERCENTAGE)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype=sTypePercentage;
		gDevice.id=ID4;
		gDevice.floatval1=pDevice->floatValue;
		gDevice.intval1=(int)(ID1<<24)|(ID2<<16)|(ID3<<8)|ID4;
		sDecodeRXMessage(this, (const unsigned char *)&gDevice);
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_AMPERE)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.CURRENT.packettype=pTypeCURRENT;
		tsen.CURRENT.subtype=sTypeELEC1;
		tsen.CURRENT.packetlength=sizeof(tsen.CURRENT)-1;
		tsen.CURRENT.id1=ID3;
		tsen.CURRENT.id2=ID4;
		int amps=round(pDevice->floatValue*10.0f);
		tsen.CURRENT.ch1h=amps/256;
		amps-=(tsen.CURRENT.ch1h*256);
		tsen.CURRENT.ch1l=(BYTE)amps;
		tsen.CURRENT.battery_level=9;
		tsen.CURRENT.rssi=12;
		if (pDevice->hasBattery)
		{
			tsen.CURRENT.battery_level=Convert_Battery_To_PercInt(pDevice->batValue);
		}
		sDecodeRXMessage(this, (const unsigned char *)&tsen.CURRENT);
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_POWERENERGYMETER)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));

		const _tZWaveDevice *pPowerDevice=NULL;
		pPowerDevice=FindDevice(pDevice->nodeID,pDevice->instanceID,pDevice->indexID,ZDTYPE_SENSOR_POWER);
		if (pPowerDevice==NULL)
			pPowerDevice=FindDevice(pDevice->nodeID,-1,-1,ZDTYPE_SENSOR_POWER);
		if (pPowerDevice)
		{
			tsen.ENERGY.packettype=pTypeENERGY;
			tsen.ENERGY.subtype=sTypeELEC2;
			tsen.ENERGY.packetlength=sizeof(tsen.ENERGY)-1;
			tsen.ENERGY.id1=ID3;
			tsen.ENERGY.id2=ID4;
			tsen.ENERGY.count=1;
			tsen.ENERGY.rssi=12;

			tsen.ENERGY.battery_level=9;
			if (pDevice->hasBattery)
			{
				tsen.ENERGY.battery_level=Convert_Battery_To_PercInt(pDevice->batValue);
			}

			unsigned long long instant=(unsigned long long)round(pPowerDevice->floatValue);
			tsen.ENERGY.instant1=(unsigned char)(instant/0x1000000);
			instant-=tsen.ENERGY.instant1*0x1000000;
			tsen.ENERGY.instant2=(unsigned char)(instant/0x10000);
			instant-=tsen.ENERGY.instant2*0x10000;
			tsen.ENERGY.instant3=(unsigned char)(instant/0x100);
			instant-=tsen.ENERGY.instant3*0x100;
			tsen.ENERGY.instant4=(unsigned char)(instant);

			double total=pDevice->floatValue*223.666;
			tsen.ENERGY.total1=(unsigned char)(total/0x10000000000ULL);
			total-=tsen.ENERGY.total1*0x10000000000ULL;
			tsen.ENERGY.total2=(unsigned char)(total/0x100000000ULL);
			total-=tsen.ENERGY.total2*0x100000000ULL;
			tsen.ENERGY.total3=(unsigned char)(total/0x1000000);
			total-=tsen.ENERGY.total3*0x1000000;
			tsen.ENERGY.total4=(unsigned char)(total/0x10000);
			total-=tsen.ENERGY.total4*0x10000;
			tsen.ENERGY.total5=(unsigned char)(total/0x100);
			total-=tsen.ENERGY.total5*0x100;
			tsen.ENERGY.total6=(unsigned char)(total);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);//decode message
		}
		else
		{
			tsen.ENERGY.packettype=pTypeENERGY;
			tsen.ENERGY.subtype=sTypeELEC2;
			tsen.ENERGY.packetlength=sizeof(tsen.ENERGY)-1;
			tsen.ENERGY.id1=ID3;
			tsen.ENERGY.id2=ID4;
			tsen.ENERGY.count=1;
			tsen.ENERGY.rssi=12;

			tsen.ENERGY.battery_level=9;
			if (pDevice->hasBattery)
			{
				tsen.ENERGY.battery_level=Convert_Battery_To_PercInt(pDevice->batValue);
			}

			unsigned long long instant=0;
			tsen.ENERGY.instant1=(unsigned char)(instant/0x1000000);
			instant-=tsen.ENERGY.instant1*0x1000000;
			tsen.ENERGY.instant2=(unsigned char)(instant/0x10000);
			instant-=tsen.ENERGY.instant2*0x10000;
			tsen.ENERGY.instant3=(unsigned char)(instant/0x100);
			instant-=tsen.ENERGY.instant3*0x100;
			tsen.ENERGY.instant4=(unsigned char)(instant);

			double total=pDevice->floatValue*223.666;
			tsen.ENERGY.total1=(unsigned char)(total/0x10000000000ULL);
			total-=tsen.ENERGY.total1*0x10000000000ULL;
			tsen.ENERGY.total2=(unsigned char)(total/0x100000000ULL);
			total-=tsen.ENERGY.total2*0x100000000ULL;
			tsen.ENERGY.total3=(unsigned char)(total/0x1000000);
			total-=tsen.ENERGY.total3*0x1000000;
			tsen.ENERGY.total4=(unsigned char)(total/0x10000);
			total-=tsen.ENERGY.total4*0x10000;
			tsen.ENERGY.total5=(unsigned char)(total/0x100);
			total-=tsen.ENERGY.total5*0x100;
			tsen.ENERGY.total6=(unsigned char)(total);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);//decode message
		}
	}
	else if (pDevice->devType==ZDTYPE_SENSOR_TEMPERATURE)
	{
		if (!pDevice->bValidValue)
			return;
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));

		const _tZWaveDevice *pHumDevice=FindDevice(pDevice->nodeID,-1,-1,ZDTYPE_SENSOR_HUMIDITY);
		if (pHumDevice)
		{
			if (!pHumDevice->bValidValue)
				return;
			tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
			tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
			tsen.TEMP_HUM.subtype=sTypeTH5;
			tsen.TEMP_HUM.rssi=12;
			tsen.TEMP_HUM.id1=ID3;
			tsen.TEMP_HUM.id2=ID4;

			tsen.TEMP_HUM.battery_level=9;
			if (pDevice->hasBattery)
			{
				tsen.TEMP_HUM.battery_level=Convert_Battery_To_PercInt(pDevice->batValue);
			}

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
			tsen.TEMP.rssi=12;
			tsen.TEMP.id1=ID3;
			tsen.TEMP.id2=ID4;

			tsen.TEMP.battery_level=9;
			if (pDevice->hasBattery)
			{
				tsen.TEMP.battery_level=Convert_Battery_To_PercInt(pDevice->batValue);
			}

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
		if (!pDevice->bValidValue)
			return;
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));

		const _tZWaveDevice *pTempDevice=FindDevice(pDevice->nodeID,-1,-1,ZDTYPE_SENSOR_TEMPERATURE);
		if (pTempDevice)
		{
			if (!pTempDevice->bValidValue)
				return;

			//report it with the ID of the temperature sensor, else we get two sensors with the same value
			ID1 = 0;
			ID2 = (unsigned char)((pTempDevice->nodeID & 0xFF00) >> 8);
			ID3 = (unsigned char)pTempDevice->nodeID & 0xFF;
			ID4 = pTempDevice->instanceID;

			tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
			tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
			tsen.TEMP_HUM.subtype=sTypeTH5;
			tsen.TEMP_HUM.rssi=12;
			tsen.TEMP_HUM.id1=ID3;
			tsen.TEMP_HUM.id2=ID4;
			ID4=pTempDevice->instanceID;

			tsen.TEMP_HUM.battery_level=9;
			if (pDevice->hasBattery)
			{
				tsen.TEMP_HUM.battery_level=Convert_Battery_To_PercInt(pDevice->batValue);
			}

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
			tsen.HUM.rssi=12;
			tsen.HUM.id1=ID3;
			tsen.HUM.id2=ID4;
			tsen.HUM.battery_level=9;
			if (pDevice->hasBattery)
			{
				tsen.HUM.battery_level=Convert_Battery_To_PercInt(pDevice->batValue);
			}
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
		lmeter.battery_level=255;
		if (pDevice->hasBattery)
			lmeter.battery_level=pDevice->batValue;
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
		tmeter.battery_level=255;
		if (pDevice->hasBattery)
			tmeter.battery_level=pDevice->batValue;
		tmeter.temp=pDevice->floatValue;
		sDecodeRXMessage(this, (const unsigned char *)&tmeter);
	}
}

ZWaveBase::_tZWaveDevice* ZWaveBase::FindDevice(const int nodeID, const int instanceID, const int indexID, const _eZWaveDeviceType devType)
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

ZWaveBase::_tZWaveDevice* ZWaveBase::FindDevice(const int nodeID, const int instanceID, const int indexID, const int CommandClassID,  const _eZWaveDeviceType devType)
{
	std::map<std::string,_tZWaveDevice>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		if (
			(itt->second.nodeID==nodeID)&&
			((itt->second.instanceID==instanceID)||(instanceID==-1))&&
			(itt->second.commandClassID==CommandClassID)&&
			(itt->second.devType==devType)
			)
			return &itt->second;
	}
	return NULL;
}

void hue2rgb(const float hue, int &outR, int &outG, int &outB)
{
	double      hh, p, q, t, ff;
	long        i;
	hh = hue;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	double saturation = 1.0;
	double vlue = 1.0;
	p = vlue * (1.0 - saturation);
	q = vlue * (1.0 - (saturation * ff));
	t = vlue * (1.0 - (saturation * (1.0 - ff)));

	switch (i) {
	case 0:
		outR = int(vlue*100.0);
		outG = int(t*100.0);
		outB = int(p*100.0);
		break;
	case 1:
		outR = int(q*100.0);
		outG = int(vlue*100.0);
		outB = int(p*100.0);
		break;
	case 2:
		outR = int(p*100.0);
		outG = int(vlue*100.0);
		outB = int(t*100.0);
		break;

	case 3:
		outR = int(p*100.0);
		outG = int(q*100.0);
		outB = int(vlue*100.0);
		break;
	case 4:
		outR = int(t*100.0);
		outG = int(p*100.0);
		outB = int(vlue*100.0);
		break;
	case 5:
	default:
		outR = int(vlue*100.0);
		outG = int(p*100.0);
		outB = int(q*100.0);
		break;
	}
}

void ZWaveBase::WriteToHardware(const char *pdata, const unsigned char length)
{
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
		int indexID=pSen->LIGHTING2.id1;

		int svalue=0;

		//First find dimmer
		pDevice=FindDevice(nodeID,instanceID,indexID,ZDTYPE_SWITCH_DIMMER);
		if (pDevice)
		{
			if ((pSen->LIGHTING2.cmnd==light2_sOff)||(pSen->LIGHTING2.cmnd==light2_sGroupOff))
				svalue=0;
			else if ((pSen->LIGHTING2.cmnd==light2_sOn)||(pSen->LIGHTING2.cmnd==light2_sGroupOn))
				svalue=255;
			else
			{
				float fvalue=(100.0f/15.0f)*float(pSen->LIGHTING2.level);
				if (fvalue>99.0f)
					fvalue=99.0f; //99 is fully on
				svalue=round(fvalue);
			}
			SwitchLight(nodeID,instanceID,pDevice->commandClassID,svalue);
		}
		else
		{
			//find normal
			pDevice=FindDevice(nodeID,instanceID,indexID,ZDTYPE_SWITCH_NORMAL);
			if (pDevice)
			{
				if ((pSen->LIGHTING2.cmnd==light2_sOff)||(pSen->LIGHTING2.cmnd==light2_sGroupOff))
					svalue=0;
				else 
					svalue=255;
				SwitchLight(nodeID,instanceID,pDevice->commandClassID,svalue);
			}
		}
	}
	else if ((packettype==pTypeThermostat)&&(subtype==sTypeThermSetpoint))
	{
		//Set Point
		_tThermostat *pMeter=(_tThermostat *)pSen;

		int nodeID=(pMeter->id2<<8)|pMeter->id3;
		int instanceID=pMeter->id4;
		int indexID=pMeter->id1;

		//find normal
		pDevice=FindDevice(nodeID,instanceID,indexID,ZDTYPE_SENSOR_SETPOINT);
		if (pDevice)
		{
			SetThermostatSetPoint(nodeID,instanceID,pDevice->commandClassID,pMeter->temp);
		}
	}
	else if (packettype == pTypeLimitlessLights)
	{
		_tLimitlessLights *pLed = (_tLimitlessLights*)pSen;
		unsigned char ID1 = (unsigned char)((pLed->id & 0xFF000000) >> 24);
		unsigned char ID2 = (unsigned char)((pLed->id & 0x00FF0000) >> 16);
		unsigned char ID3 = (unsigned char)((pLed->id & 0x0000FF00) >> 8);
		unsigned char ID4 = (unsigned char)pLed->id & 0x000000FF;
		int nodeID = (ID2 << 8) | ID3;
		int instanceID = ID4;
		int indexID = ID1;
		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SWITCH_FGRGBWM441);
		if (pDevice)
		{
			int svalue = 0;
			if (pLed->command == Limitless_LedOff)
			{
				instanceID = 2;
				svalue = 0;
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
			}
			else if (pLed->command == Limitless_LedOn)
			{
				instanceID = 2;
				svalue = 255;
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
			}
			else if (pLed->command == Limitless_SetBrightnessLevel)
			{
				instanceID = 2;
				float fvalue = pLed->value;
				if (fvalue > 99.0f)
					fvalue = 99.0f; //99 is fully on
				svalue = round(fvalue);
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
			}
			else if (pLed->command == Limitless_SetColorToWhite)
			{
				instanceID = 3;//red
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0);
				instanceID = 4;//green
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0);
				instanceID = 5;//blue
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0);
				instanceID = 6;//white
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, 100);
				return;
			}
			else if (pLed->command == Limitless_SetRGBColour)
			{
				int red, green, blue;
				float cHue = (360.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
				hue2rgb(cHue, red, green, blue);

				instanceID = 6;//white
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0);

				instanceID = 3;//red
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, red);
				instanceID = 4;//green
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, green);
				instanceID = 5;//blue
				SwitchLight(nodeID, instanceID, pDevice->commandClassID, blue);
				_log.Log( LOG_NORM, "Red: %03d, Green:%03d, Blue:%03d", red, green, blue);
				return;
			}
		}
	}
}

