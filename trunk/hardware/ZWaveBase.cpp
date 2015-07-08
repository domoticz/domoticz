#include "stdafx.h"
#include "ZWaveBase.h"
#include "ZWaveCommands.h"

#include <sstream>      // std::stringstream
#include <vector>
#include <ctype.h>
#include <iomanip>

#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"

#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"

#include "OpenZWave.h"

#define CONTROLLER_COMMAND_TIMEOUT 20

#pragma warning(disable: 4996)

#define round(a) ( int ) ( a + .5 )

extern std::string szWWWFolder;

ZWaveBase::ZWaveBase()
{
	m_LastIncludedNode=0;
	m_bControllerCommandInProgress=false;
	m_bControllerCommandCanceled = false;
	m_updateTime = 0;
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
	m_bControllerCommandCanceled = false;
	m_bIsStarted = true;

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
	int msec_counter = 0;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_milliseconds(500);
		if (m_stoprequested)
			return;
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0) {
				m_LastHeartbeat=mytime(NULL);
			}
		}


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
					_log.Log(LOG_STATUS,"ZWave: Stopping Controller command (Timeout!)");
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
		_log.Log(LOG_NORM, "New device: %s", device.string_id.c_str());
	}
	else
	{
		_log.Log(LOG_NORM, "Update device: %s", device.string_id.c_str());
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

bool ZWaveBase::IsNodeRGBW(const unsigned int homeID, const int nodeID)
{
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ProductDescription FROM ZWaveNodes WHERE (HardwareID==" << m_HwdID << ") AND (HomeID==" << homeID << ") AND (NodeID==" << nodeID << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
		return false;
	std::string ProductDescription = result[0][0];
	return (ProductDescription.find("FGRGBWM441") != std::string::npos);
}

void ZWaveBase::SendSwitchIfNotExists(const _tZWaveDevice *pDevice)
{
	if (
		(pDevice->devType!=ZDTYPE_SWITCH_NORMAL)&&
		(pDevice->devType != ZDTYPE_SWITCH_DIMMER) &&
		(pDevice->devType != ZDTYPE_SWITCH_FGRGBWM441)&&
		(pDevice->devType != ZDTYPE_SWITCH_COLOR)
		)
		return; //only for switches

	m_iLastSendNodeBatteryValue = 255;
	if (pDevice->hasBattery)
	{
		m_iLastSendNodeBatteryValue = pDevice->batValue;
	}

	if ((pDevice->devType == ZDTYPE_SWITCH_FGRGBWM441) || (pDevice->devType == ZDTYPE_SWITCH_COLOR))
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
		szQuery << "UPDATE DeviceStatus SET Name='" << pDevice->label << "', SwitchType=" << STYPE_Dimmer << " WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << ID << "')";
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
		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==" << int(unitcode) << ") AND (Type==" << pTypeLighting2 << ") AND (SubType==" << sTypeZWaveSwitch << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
		if (result.size() > 0)
			return; //Already in the system

		//Send as Lighting 2

		tRBUF lcmd;
		memset(&lcmd, 0, sizeof(RBUF));
		lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
		lcmd.LIGHTING2.packettype = pTypeLighting2;
		lcmd.LIGHTING2.subtype = sTypeZWaveSwitch;
		lcmd.LIGHTING2.seqnbr = pDevice->sequence_number;
		lcmd.LIGHTING2.id1 = ID1;
		lcmd.LIGHTING2.id2 = ID2;
		lcmd.LIGHTING2.id3 = ID3;
		lcmd.LIGHTING2.id4 = ID4;
		lcmd.LIGHTING2.unitcode = unitcode;

		// Get device level to set
		int level = pDevice->intvalue;

		// Simple on/off device, make sure we only have 0 or 255
		if (pDevice->devType == ZDTYPE_SWITCH_NORMAL)
			level = (level == 0) ? 0 : 255;

		// Now check the values
		if (level == 0)
			lcmd.LIGHTING2.cmnd = light2_sOff;
		else if (level == 255)
			lcmd.LIGHTING2.cmnd = light2_sOn;
		else
		{
			level = (level > 99) ? 99 : level;
			lcmd.LIGHTING2.cmnd = light2_sSetLevel;
		}

		// Update the lcmd with the correct level value.
		lcmd.LIGHTING2.level=level;

		lcmd.LIGHTING2.filler = 0;
		lcmd.LIGHTING2.rssi = 12;

		sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);

		int SwitchType = (pDevice->devType == ZDTYPE_SWITCH_DIMMER) ? 7 : 0;

		//Set Name
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << pDevice->label << "', SwitchType=" << SwitchType << " WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << ID << "')";
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

	unsigned long lID = (ID1 << 24) + (ID2 << 16) + (ID3 << 8) + ID4;

	m_iLastSendNodeBatteryValue = 255;
	if (pDevice->hasBattery)
	{
		m_iLastSendNodeBatteryValue = pDevice->batValue;
	}

	if ((pDevice->devType==ZDTYPE_SWITCH_NORMAL)||(pDevice->devType==ZDTYPE_SWITCH_DIMMER))
	{
		//Send as Lighting 2
		tRBUF lcmd;
		memset(&lcmd,0,sizeof(RBUF));
		lcmd.LIGHTING2.packetlength=sizeof(lcmd.LIGHTING2)-1;
		lcmd.LIGHTING2.packettype=pTypeLighting2;
		lcmd.LIGHTING2.subtype=sTypeZWaveSwitch;
		lcmd.LIGHTING2.seqnbr=pDevice->sequence_number;
		lcmd.LIGHTING2.id1=ID1;
		lcmd.LIGHTING2.id2=ID2;
		lcmd.LIGHTING2.id3=ID3;
		lcmd.LIGHTING2.id4=ID4;
		lcmd.LIGHTING2.unitcode=1;

		// Get device level to set
		int level = pDevice->intvalue;

		// Simple on/off device, make sure we only have 0 or 255
		if (pDevice->devType == ZDTYPE_SWITCH_NORMAL)
			level = (level == 0) ? 0 : 255;

		// Now check the values
		if (level == 0)
			lcmd.LIGHTING2.cmnd = light2_sOff;
		else if (level == 255)
			lcmd.LIGHTING2.cmnd = light2_sOn;
		else
		{
			level = (level > 99) ? 99 : level;
			lcmd.LIGHTING2.cmnd = light2_sSetLevel;
		}

		// Update the lcmd with the correct level value.
		lcmd.LIGHTING2.level=level;
		lcmd.LIGHTING2.filler=0;
		lcmd.LIGHTING2.rssi=12;
		sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);//decode message
		return;
	}
	else if ((pDevice->devType == ZDTYPE_SWITCH_FGRGBWM441) || (pDevice->devType == ZDTYPE_SWITCH_COLOR))
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
		sprintf(szID, "%08x", (unsigned int)lID);
		std::string ID = szID;
		unsigned char unitcode = 1;

		//Send as LimitlessLight
		_tLimitlessLights lcmd;
		lcmd.id = lID;

		// Get device level to set
		int level = pDevice->intvalue;

		if (level == 0)
			lcmd.command = Limitless_LedOff;
		else
			lcmd.command = Limitless_LedOn;
		lcmd.value = 0;
		sDecodeRXMessage(this, (const unsigned char *)&lcmd);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_POWER)
	{
		_tUsageMeter umeter;
		umeter.id1 = ID1;
		umeter.id2 = ID2;
		umeter.id3 = ID3;
		umeter.id4 = ID4;
		umeter.dunit = pDevice->scaleID;
		umeter.fusage = pDevice->floatValue;
		sDecodeRXMessage(this, (const unsigned char *)&umeter);//decode message

		//Search Energy Device
		const _tZWaveDevice *pEnergyDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWERENERGYMETER);
		if (pEnergyDevice == NULL)
		{
			pEnergyDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, ZDTYPE_SENSOR_POWERENERGYMETER);
			//if (pEnergyDevice == NULL)
				//pEnergyDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_POWERENERGYMETER);
		}
		if (pEnergyDevice)
		{
			RBUF tsen;
			memset(&tsen, 0, sizeof(RBUF));
			tsen.ENERGY.packettype = pTypeENERGY;
			tsen.ENERGY.subtype = sTypeELEC2;
			tsen.ENERGY.packetlength = sizeof(tsen.ENERGY) - 1;
			tsen.ENERGY.id1 = ID3;
			tsen.ENERGY.id2 = ID4;
			tsen.ENERGY.count = 1;
			tsen.ENERGY.rssi = 12;

			tsen.ENERGY.battery_level = 9;
			if (pDevice->hasBattery)
			{
				tsen.ENERGY.battery_level = Convert_Battery_To_PercInt(pDevice->batValue);
			}

			unsigned long long instant = (unsigned long long)round(pDevice->floatValue);
			tsen.ENERGY.instant1 = (unsigned char)(instant / 0x1000000);
			instant -= tsen.ENERGY.instant1 * 0x1000000;
			tsen.ENERGY.instant2 = (unsigned char)(instant / 0x10000);
			instant -= tsen.ENERGY.instant2 * 0x10000;
			tsen.ENERGY.instant3 = (unsigned char)(instant / 0x100);
			instant -= tsen.ENERGY.instant3 * 0x100;
			tsen.ENERGY.instant4 = (unsigned char)(instant);

			double total = pEnergyDevice->floatValue*223.666;
			tsen.ENERGY.total1 = (unsigned char)(total / 0x10000000000ULL);
			total -= tsen.ENERGY.total1 * 0x10000000000ULL;
			tsen.ENERGY.total2 = (unsigned char)(total / 0x100000000ULL);
			total -= tsen.ENERGY.total2 * 0x100000000ULL;
			tsen.ENERGY.total3 = (unsigned char)(total / 0x1000000);
			total -= tsen.ENERGY.total3 * 0x1000000;
			tsen.ENERGY.total4 = (unsigned char)(total / 0x10000);
			total -= tsen.ENERGY.total4 * 0x10000;
			tsen.ENERGY.total5 = (unsigned char)(total / 0x100);
			total -= tsen.ENERGY.total5 * 0x100;
			tsen.ENERGY.total6 = (unsigned char)(total);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);//decode message

		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_POWERENERGYMETER)
	{
		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));

		const _tZWaveDevice *pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWER);
		if (pPowerDevice == NULL)
		{
			pPowerDevice=FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, ZDTYPE_SENSOR_POWER);
			if (pPowerDevice == NULL)
				pPowerDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_POWER);
		}
		if (pPowerDevice)
		{
			tsen.ENERGY.packettype = pTypeENERGY;
			tsen.ENERGY.subtype = sTypeELEC2;
			tsen.ENERGY.packetlength = sizeof(tsen.ENERGY) - 1;
			tsen.ENERGY.id1 = ID3;
			tsen.ENERGY.id2 = ID4;
			tsen.ENERGY.count = 1;
			tsen.ENERGY.rssi = 12;

			tsen.ENERGY.battery_level = 9;
			if (pDevice->hasBattery)
			{
				tsen.ENERGY.battery_level = Convert_Battery_To_PercInt(pDevice->batValue);
			}

			unsigned long long instant = (unsigned long long)round(pPowerDevice->floatValue);
			tsen.ENERGY.instant1 = (unsigned char)(instant / 0x1000000);
			instant -= tsen.ENERGY.instant1 * 0x1000000;
			tsen.ENERGY.instant2 = (unsigned char)(instant / 0x10000);
			instant -= tsen.ENERGY.instant2 * 0x10000;
			tsen.ENERGY.instant3 = (unsigned char)(instant / 0x100);
			instant -= tsen.ENERGY.instant3 * 0x100;
			tsen.ENERGY.instant4 = (unsigned char)(instant);

			double total = pDevice->floatValue*223.666;
			tsen.ENERGY.total1 = (unsigned char)(total / 0x10000000000ULL);
			total -= tsen.ENERGY.total1 * 0x10000000000ULL;
			tsen.ENERGY.total2 = (unsigned char)(total / 0x100000000ULL);
			total -= tsen.ENERGY.total2 * 0x100000000ULL;
			tsen.ENERGY.total3 = (unsigned char)(total / 0x1000000);
			total -= tsen.ENERGY.total3 * 0x1000000;
			tsen.ENERGY.total4 = (unsigned char)(total / 0x10000);
			total -= tsen.ENERGY.total4 * 0x10000;
			tsen.ENERGY.total5 = (unsigned char)(total / 0x100);
			total -= tsen.ENERGY.total5 * 0x100;
			tsen.ENERGY.total6 = (unsigned char)(total);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);//decode message
		}
		else
		{
			tsen.ENERGY.packettype = pTypeENERGY;
			tsen.ENERGY.subtype = sTypeELEC2;
			tsen.ENERGY.packetlength = sizeof(tsen.ENERGY) - 1;
			tsen.ENERGY.id1 = ID3;
			tsen.ENERGY.id2 = ID4;
			tsen.ENERGY.count = 1;
			tsen.ENERGY.rssi = 12;

			tsen.ENERGY.battery_level = 9;
			if (pDevice->hasBattery)
			{
				tsen.ENERGY.battery_level = Convert_Battery_To_PercInt(pDevice->batValue);
			}

			unsigned long long instant = 0;
			tsen.ENERGY.instant1 = (unsigned char)(instant / 0x1000000);
			instant -= tsen.ENERGY.instant1 * 0x1000000;
			tsen.ENERGY.instant2 = (unsigned char)(instant / 0x10000);
			instant -= tsen.ENERGY.instant2 * 0x10000;
			tsen.ENERGY.instant3 = (unsigned char)(instant / 0x100);
			instant -= tsen.ENERGY.instant3 * 0x100;
			tsen.ENERGY.instant4 = (unsigned char)(instant);

			double total = pDevice->floatValue*223.666;
			tsen.ENERGY.total1 = (unsigned char)(total / 0x10000000000ULL);
			total -= tsen.ENERGY.total1 * 0x10000000000ULL;
			tsen.ENERGY.total2 = (unsigned char)(total / 0x100000000ULL);
			total -= tsen.ENERGY.total2 * 0x100000000ULL;
			tsen.ENERGY.total3 = (unsigned char)(total / 0x1000000);
			total -= tsen.ENERGY.total3 * 0x1000000;
			tsen.ENERGY.total4 = (unsigned char)(total / 0x10000);
			total -= tsen.ENERGY.total4 * 0x10000;
			tsen.ENERGY.total5 = (unsigned char)(total / 0x100);
			total -= tsen.ENERGY.total5 * 0x100;
			tsen.ENERGY.total6 = (unsigned char)(total);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);//decode message
		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_VOLTAGE)
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
	else if (pDevice->devType == ZDTYPE_SENSOR_VELOCITY)
	{
		if (!pDevice->bValidValue)
			return;

		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));
		tsen.WIND.packetlength = sizeof(tsen.WIND) - 1;
		tsen.WIND.packettype = pTypeWIND;
		tsen.WIND.subtype = sTypeWIND4;
		tsen.WIND.battery_level = 9;
		if (pDevice->hasBattery)
		{
			tsen.WIND.battery_level = Convert_Battery_To_PercInt(pDevice->batValue);
		}
		tsen.WIND.rssi = 12;
		tsen.WIND.id1 = ID3;
		tsen.WIND.id2 = ID4;

		float winddir = 0;
		int aw = round(winddir);
		tsen.WIND.directionh = (BYTE)(aw / 256);
		aw -= (tsen.WIND.directionh * 256);
		tsen.WIND.directionl = (BYTE)(aw);

		int sw = round(pDevice->floatValue*10.0f);
		tsen.WIND.av_speedh = (BYTE)(sw / 256);
		sw -= (tsen.WIND.av_speedh * 256);
		tsen.WIND.av_speedl = (BYTE)(sw);
		tsen.WIND.gusth = tsen.WIND.av_speedh;
		tsen.WIND.gustl = tsen.WIND.av_speedl;

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh = 0;
		tsen.WIND.chilll = 0;
		tsen.WIND.temperatureh = 0;
		tsen.WIND.temperaturel = 0;

		const _tZWaveDevice *pTempDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_TEMPERATURE);
		if (pTempDevice)
		{
			if (!pTempDevice->bValidValue)
				return;
			tsen.WIND.tempsign = (pTempDevice->floatValue >= 0) ? 0 : 1;
			tsen.WIND.chillsign = (pTempDevice->floatValue >= 0) ? 0 : 1;
			int at10 = round(abs(pTempDevice->floatValue*10.0f));
			tsen.WIND.temperatureh = (BYTE)(at10 / 256);
			tsen.WIND.chillh = (BYTE)(at10 / 256);
			at10 -= (tsen.WIND.chillh * 256);
			tsen.WIND.temperaturel = (BYTE)(at10);
			tsen.WIND.chilll = (BYTE)(at10);
		}
		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND);//decode message
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_BAROMETER)
	{
		if (!pDevice->bValidValue)
			return;
		const _tZWaveDevice *pTempDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_TEMPERATURE);
		const _tZWaveDevice *pHumDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_HUMIDITY);
		if (pTempDevice && pHumDevice)
		{
			int nforecast = wsbaroforcast_some_clouds;
			float pressure = pDevice->floatValue;
			if (pressure <= 980)
				nforecast = wsbaroforcast_heavy_rain;
			else if (pressure <= 995)
			{
				if (pDevice->floatValue > 1)
					nforecast = wsbaroforcast_rain;
				else
					nforecast = wsbaroforcast_snow;
			}
			else if (pressure >= 1029)
				nforecast = wsbaroforcast_sunny;
			SendTempHumBaroSensorFloat(pDevice->nodeID, pDevice->batValue, pTempDevice->floatValue, pHumDevice->intvalue, pDevice->floatValue, nforecast, "TempHumBaro");
		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_LIGHT)
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
	else if (pDevice->devType == ZDTYPE_SENSOR_GAS)
	{
		P1Gas	m_p1gas;
		m_p1gas.type = pTypeP1Gas;
		m_p1gas.subtype = sTypeP1Gas;
		m_p1gas.gasusage = (unsigned long)(pDevice->floatValue * 1000);
		m_p1gas.ID = lID;
		sDecodeRXMessage(this, (const unsigned char *)&m_p1gas);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_CO2)
	{
		SendAirQualitySensor(ID3, ID4, pDevice->batValue, int(pDevice->floatValue), "CO2 Sensor");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_SETPOINT)
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
	else if (pDevice->devType == ZDTYPE_SENSOR_THERMOSTAT_CLOCK)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveClock;
		gDevice.id = ID4;
		gDevice.intval1 = (int)(ID1 << 24) | (ID2 << 16) | (ID3 << 8) | ID4;
		gDevice.intval2 = pDevice->intvalue;
		sDecodeRXMessage(this, (const unsigned char *)&gDevice);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_THERMOSTAT_MODE)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveThermostatMode;
		gDevice.id = ID4;
		gDevice.intval1 = (int)(ID1 << 24) | (ID2 << 16) | (ID3 << 8) | ID4;
		gDevice.intval2 = pDevice->intvalue;
		sDecodeRXMessage(this, (const unsigned char *)&gDevice);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveThermostatFanMode;
		gDevice.id = ID4;
		gDevice.intval1 = (int)(ID1 << 24) | (ID2 << 16) | (ID3 << 8) | ID4;
		gDevice.intval2 = pDevice->intvalue;
		sDecodeRXMessage(this, (const unsigned char *)&gDevice);
	}
}

ZWaveBase::_tZWaveDevice* ZWaveBase::FindDevice(const int nodeID, const int instanceID, const int indexID)
{
	std::map<std::string, _tZWaveDevice>::iterator itt;
	for (itt = m_devices.begin(); itt != m_devices.end(); ++itt)
	{
		if (
			(itt->second.nodeID == nodeID) &&
			(itt->second.instanceID == instanceID) &&
			(itt->second.indexID == indexID)
			)
			return &itt->second;
	}
	return NULL;
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

bool ZWaveBase::WriteToHardware(const char *pdata, const unsigned char length)
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
				svalue = (pSen->LIGHTING2.level<99) ? pSen->LIGHTING2.level : 99;
			}
			return SwitchLight(nodeID,instanceID,pDevice->commandClassID,svalue);
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
				return SwitchLight(nodeID,instanceID,pDevice->commandClassID,svalue);
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
	else if ((packettype == pTypeGeneral) && (subtype == sTypeZWaveClock))
	{
		_tGeneralDevice *pMeter = (_tGeneralDevice*)pSen;
		unsigned char ID1 = (unsigned char)((pMeter->intval1 & 0xFF000000) >> 24);
		unsigned char ID2 = (unsigned char)((pMeter->intval1 & 0x00FF0000) >> 16);
		unsigned char ID3 = (unsigned char)((pMeter->intval1 & 0x0000FF00) >> 8);
		unsigned char ID4 = (unsigned char)((pMeter->intval1 & 0x000000FF));

		int nodeID = (ID2 << 8) | ID3;
		int instanceID = ID4;
		int indexID = ID1;

		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_CLOCK);
		if (pDevice)
		{
			int tintval = pMeter->intval2;
			int day = tintval / (24 * 60); tintval -= (day * 24 * 60);
			int hour = tintval / (60); tintval -= (hour * 60);
			int minute = tintval;

			SetClock(nodeID, instanceID, pDevice->commandClassID, day, hour, minute);
		}
	}
	else if ((packettype == pTypeGeneral) && (subtype == sTypeZWaveThermostatMode))
	{
		_tGeneralDevice *pMeter = (_tGeneralDevice*)pSen;
		unsigned char ID1 = (unsigned char)((pMeter->intval1 & 0xFF000000) >> 24);
		unsigned char ID2 = (unsigned char)((pMeter->intval1 & 0x00FF0000) >> 16);
		unsigned char ID3 = (unsigned char)((pMeter->intval1 & 0x0000FF00) >> 8);
		unsigned char ID4 = (unsigned char)((pMeter->intval1 & 0x000000FF));

		int nodeID = (ID2 << 8) | ID3;
		int instanceID = ID4;
		int indexID = ID1;

		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_MODE);
		if (pDevice)
		{
			int tMode = pMeter->intval2;
			SetThermostatMode(nodeID, instanceID, pDevice->commandClassID, tMode);
		}
	}
	else if ((packettype == pTypeGeneral) && (subtype == sTypeZWaveThermostatFanMode))
	{
		_tGeneralDevice *pMeter = (_tGeneralDevice*)pSen;
		unsigned char ID1 = (unsigned char)((pMeter->intval1 & 0xFF000000) >> 24);
		unsigned char ID2 = (unsigned char)((pMeter->intval1 & 0x00FF0000) >> 16);
		unsigned char ID3 = (unsigned char)((pMeter->intval1 & 0x0000FF00) >> 8);
		unsigned char ID4 = (unsigned char)((pMeter->intval1 & 0x000000FF));

		int nodeID = (ID2 << 8) | ID3;
		int instanceID = ID4;
		int indexID = ID1;

		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE);
		if (pDevice)
		{
			int tMode = pMeter->intval2;
			SetThermostatFanMode(nodeID, instanceID, pDevice->commandClassID, tMode);
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
				return SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
			}
			else if (pLed->command == Limitless_LedOn)
			{
				instanceID = 2;
				svalue = 255;
				return SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
			}
			else if (pLed->command == Limitless_SetBrightnessLevel)
			{
				instanceID = 2;
				float fvalue = pLed->value;
				if (fvalue > 99.0f)
					fvalue = 99.0f; //99 is fully on
				svalue = round(fvalue);
				return SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
			}
			else if (pLed->command == Limitless_SetColorToWhite)
			{
				instanceID = 3;//red
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0))
					return false;
				instanceID = 4;//green
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0))
					return false;
				instanceID = 5;//blue
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0))
					return false;
				instanceID = 6;//white
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, 100))
					return false;
				return true;
			}
			else if (pLed->command == Limitless_SetRGBColour)
			{
				int red, green, blue;
				float cHue = (360.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
				hue2rgb(cHue, red, green, blue);

				instanceID = 6;//white
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, 0))
					return false;

				instanceID = 3;//red
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, red))
					return false;
				instanceID = 4;//green
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, green))
					return false;
				instanceID = 5;//blue
				if (!SwitchLight(nodeID, instanceID, pDevice->commandClassID, blue))
					return false;
				_log.Log( LOG_NORM, "Red: %03d, Green:%03d, Blue:%03d", red, green, blue);
				return true;
			}
		}
		else
		{
			pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SWITCH_COLOR);
			if (pDevice)
			{
				std::stringstream sstr;
				int svalue = 0;
				if (pLed->command == Limitless_LedOff)
				{
					instanceID = 1;
					svalue = 0;
					return SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
				}
				else if (pLed->command == Limitless_LedOn)
				{
					instanceID = 1;
					svalue = 255;
					return SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
				}
				else if (pLed->command == Limitless_SetBrightnessLevel)
				{
					instanceID = 1;
					float fvalue = pLed->value;
					if (fvalue > 99.0f)
						fvalue = 99.0f; //99 is fully on
					svalue = round(fvalue);
					return SwitchLight(nodeID, instanceID, pDevice->commandClassID, svalue);
				}
				else if (pLed->command == Limitless_SetColorToWhite)
				{
					int Brightness = 100;
					int wWhite = round((255.0f / 100.0f)*float(Brightness));
					int cWhite = 0;
					sstr << "#000000"
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << cWhite;
					instanceID = 1;
					return SwitchColor(nodeID, instanceID, COMMAND_CLASS_COLOR_CONTROL, sstr.str());
				}
				else if (pLed->command == Limitless_SetRGBColour)
				{
					int red, green, blue;
					float cHue = (360.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255

					int Brightness = 100;

					int dMax = round((255.0f / 100.0f)*float(Brightness));
					hue2rgb(cHue, red, green, blue, dMax);
					instanceID = 1;
					int wWhite = 0;
					int cWhite = 0;
					sstr << "#"
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << red
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << green
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << blue
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << cWhite;

					std::string sColor = sstr.str();

					if (!SwitchColor(nodeID, instanceID, COMMAND_CLASS_COLOR_CONTROL, sColor))
						return false;

					_log.Log(LOG_NORM, "Red: %03d, Green:%03d, Blue:%03d", red, green, blue);
					return true;
				}
			}
		}
	}
	return true;
}

void ZWaveBase::ForceUpdateForNodeDevices(const unsigned int homeID, const int nodeID)
{
	std::map<std::string, _tZWaveDevice>::iterator itt;
	for (itt = m_devices.begin(); itt != m_devices.end(); ++itt)
	{
		if (itt->second.nodeID == nodeID)
		{
			itt->second.lastreceived = mytime(NULL)-1;

			_tZWaveDevice zdevice = itt->second;

			SendDevice2Domoticz(&zdevice);

			if (zdevice.commandClassID == COMMAND_CLASS_SWITCH_MULTILEVEL)
			{
				if (zdevice.instanceID == 1)
				{
					if (IsNodeRGBW(homeID, nodeID))
					{
						zdevice.devType = ZDTYPE_SWITCH_FGRGBWM441;
						zdevice.instanceID = 100;
						SendDevice2Domoticz(&zdevice);
					}
				}
			}
			else if (zdevice.commandClassID == COMMAND_CLASS_COLOR_CONTROL)
			{
				zdevice.devType = ZDTYPE_SWITCH_COLOR;
				zdevice.instanceID = 101;
				SendDevice2Domoticz(&zdevice);
			}

		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_OpenZWaveNodes(Json::Value &root)
		{
			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			m_ZW_Hwidx = iHardwareID;
			COpenZWave *pOZWHardware = (COpenZWave*)pHardware;

			root["status"] = "OK";
			root["title"] = "OpenZWaveNodes";

			root["NodesQueried"] = (pOZWHardware->m_awakeNodesQueried) || (pOZWHardware->m_allNodesQueried);
			root["ownNodeId"] = pOZWHardware->m_controllerNodeId;

			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID,HomeID,NodeID,Name,ProductDescription,PollTime FROM ZWaveNodes WHERE (HardwareID==" << iHardwareID << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					unsigned int homeID = atoi(sd[1].c_str());
					int nodeID = atoi(sd[2].c_str());
					//if (nodeID>1) //Don't include the controller
					{
						COpenZWave::NodeInfo *pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == NULL)
							continue;
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["HomeID"] = homeID;
						root["result"][ii]["NodeID"] = nodeID;
						root["result"][ii]["Name"] = sd[3];
						root["result"][ii]["Description"] = sd[4];
						root["result"][ii]["PollEnabled"] = (atoi(sd[5].c_str()) == 1) ? "true" : "false";
						root["result"][ii]["Version"] = pNode->iVersion;
						root["result"][ii]["Manufacturer_id"] = pNode->Manufacturer_id;
						root["result"][ii]["Manufacturer_name"] = pNode->Manufacturer_name;
						root["result"][ii]["Product_type"] = pNode->Product_type;
						root["result"][ii]["Product_id"] = pNode->Product_id;
						root["result"][ii]["Product_name"] = pNode->Product_name;
						root["result"][ii]["State"] = pOZWHardware->GetNodeStateString(homeID, nodeID);
						root["result"][ii]["HaveUserCodes"] = pNode->HaveUserCodes;
						char szDate[80];
						struct tm loctime;
						localtime_r(&pNode->m_LastSeen, &loctime);
						strftime(szDate, 80, "%Y-%m-%d %X", &loctime);

						root["result"][ii]["LastUpdate"] = szDate;

						//Add configuration parameters here
						pOZWHardware->GetNodeValuesJson(homeID, nodeID, root, ii);
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_ZWaveUpdateNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string name = m_pWebEm->FindValue("name");
			std::string senablepolling = m_pWebEm->FindValue("EnablePolling");
			if (
				(name == "") ||
				(senablepolling == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "UpdateZWaveNode";

			std::vector<std::vector<std::string> > result;
			char szTmp[300];

			sprintf(szTmp,
				"UPDATE ZWaveNodes SET Name='%s', PollTime=%d WHERE (ID==%s)",
				name.c_str(),
				(senablepolling == "true") ? 1 : 0,
				idx.c_str()
				);
			result = m_sql.query(szTmp);
			sprintf(szTmp, "SELECT HardwareID, HomeID, NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				int homeID = atoi(result[0][1].c_str());
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->SetNodeName(homeID, nodeID, name);
					pOZWHardware->EnableDisableNodePolling(nodeID);
				}
			}
		}

		void CWebServer::Cmd_ZWaveDeleteNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = atoi(result[0][1].c_str());
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RemoveFailedDevice(nodeID);
					root["status"] = "OK";
					root["title"] = "DeleteZWaveNode";
					sprintf(szTmp, "DELETE FROM ZWaveNodes WHERE (ID==%s)", idx.c_str());
					result = m_sql.query(szTmp);
				}
			}

		}

		void CWebServer::Cmd_ZWaveInclude(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string ssecure = m_pWebEm->FindValue("secure");
			bool bSecure = (ssecure == "true");
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				m_sql.AllowNewHardwareTimer(5);
				pOZWHardware->IncludeDevice(bSecure);
				root["status"] = "OK";
				root["title"] = "ZWaveInclude";
			}
		}

		void CWebServer::Cmd_ZWaveExclude(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->ExcludeDevice(1);
				root["status"] = "OK";
				root["title"] = "ZWaveExclude";
			}
		}

		void CWebServer::Cmd_ZWaveSoftReset(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->SoftResetDevice();
				root["status"] = "OK";
				root["title"] = "ZWaveSoftReset";
			}
		}

		void CWebServer::Cmd_ZWaveHardReset(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HardResetDevice();
				root["status"] = "OK";
				root["title"] = "ZWaveHardReset";
			}
		}

		void CWebServer::Cmd_ZWaveStateCheck(Json::Value &root)
		{
			root["title"] = "ZWaveStateCheck";
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				if (!pOZWHardware->GetFailedState()) {
					root["status"] = "OK";
				}
			}
			return;
		}

		void CWebServer::Cmd_ZWaveNetworkHeal(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HealNetwork();
				root["status"] = "OK";
				root["title"] = "ZWaveHealNetwork";
			}
		}

		void CWebServer::Cmd_ZWaveNodeHeal(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string node = m_pWebEm->FindValue("node");
			if (node == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HealNode(atoi(node.c_str()));
				root["status"] = "OK";
				root["title"] = "ZWaveHealNode";
			}
		}

		void CWebServer::Cmd_ZWaveNetworkInfo(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			root["title"] = "ZWaveNetworkInfo";

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			int hwID = atoi(idx.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwID);
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				std::vector< std::vector< int > > nodevectors;

				if (pOZWHardware->NetworkInfo(hwID, nodevectors)) {

					std::vector<std::vector<int> >::iterator row_iterator;
					std::vector<int>::iterator col_iterator;

					std::vector<int> allnodes;
					int rowCount = 0;
					std::stringstream allnodeslist;
					for (row_iterator = nodevectors.begin(); row_iterator != nodevectors.end(); ++row_iterator) {
						int colCount = 0;
						int nodeID = -1;
						std::vector<int> rest;
						std::stringstream list;

						for (col_iterator = (*row_iterator).begin(); col_iterator != (*row_iterator).end(); ++col_iterator) {
							if (colCount == 0) {
								nodeID = *col_iterator;
							}
							else {
								rest.push_back(*col_iterator);
							}
							colCount++;
						}

						if (nodeID != -1)
						{
							std::copy(rest.begin(), rest.end(), std::ostream_iterator<int>(list, ","));
							root["result"]["mesh"][rowCount]["nodeID"] = nodeID;
							allnodes.push_back(nodeID);
							root["result"]["mesh"][rowCount]["seesNodes"] = list.str();
							rowCount++;
						}
					}
					std::copy(allnodes.begin(), allnodes.end(), std::ostream_iterator<int>(allnodeslist, ","));
					root["result"]["nodes"] = allnodeslist.str();
					root["status"] = "OK";
				}

			}
		}

		void CWebServer::Cmd_ZWaveRemoveGroupNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string node = m_pWebEm->FindValue("node");
			if (node == "")
				return;
			std::string group = m_pWebEm->FindValue("group");
			if (group == "")
				return;
			std::string removenode = m_pWebEm->FindValue("removenode");
			if (removenode == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->RemoveNodeFromGroup(atoi(node.c_str()), atoi(group.c_str()), atoi(removenode.c_str()));
				root["status"] = "OK";
				root["title"] = "ZWaveRemoveGroupNode";
			}
		}

		void CWebServer::Cmd_ZWaveAddGroupNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::string node = m_pWebEm->FindValue("node");
			if (node == "")
				return;
			std::string group = m_pWebEm->FindValue("group");
			if (group == "")
				return;
			std::string addnode = m_pWebEm->FindValue("addnode");
			if (addnode == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->AddNodeToGroup(atoi(node.c_str()), atoi(group.c_str()), atoi(addnode.c_str()));
				root["status"] = "OK";
				root["title"] = "ZWaveAddGroupNode";
			}
		}

		void CWebServer::Cmd_ZWaveGroupInfo(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			int iHardwareID = atoi(idx.c_str());

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;

				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;
				szQuery << "SELECT ID,HomeID,NodeID,Name FROM ZWaveNodes WHERE (HardwareID==" << iHardwareID << ")";
				result = m_sql.query(szQuery.str());

				if (result.size() > 0)
				{
					int MaxNoOfGroups = 0;
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
						unsigned int homeID = atoi(sd[1].c_str());
						int nodeID = atoi(sd[2].c_str());
						COpenZWave::NodeInfo *pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == NULL)
							continue;
						std::string nodeName = sd[3].c_str();
						int numGroups = pOZWHardware->ListGroupsForNode(nodeID);
						root["result"]["nodes"][ii]["nodeID"] = nodeID;
						root["result"]["nodes"][ii]["nodeName"] = nodeName;
						root["result"]["nodes"][ii]["groupCount"] = numGroups;
						if (numGroups > 0) {
							if (numGroups > MaxNoOfGroups)
								MaxNoOfGroups = numGroups;

							std::vector< int > nodesingroup;
							int gi = 0;
							for (int x = 1; x <= numGroups; x++)
							{
								int numNodesInGroup = pOZWHardware->ListAssociatedNodesinGroup(nodeID, x, nodesingroup);
								if (numNodesInGroup > 0) {
									std::stringstream list;
									std::copy(nodesingroup.begin(), nodesingroup.end(), std::ostream_iterator<int>(list, ","));
									root["result"]["nodes"][ii]["groups"][gi]["id"] = x;
									root["result"]["nodes"][ii]["groups"][gi]["nodes"] = list.str();
								}
								else {
									root["result"]["nodes"][ii]["groups"][gi]["id"] = x;
									root["result"]["nodes"][ii]["groups"][gi]["nodes"] = "";
								}
								gi++;
								nodesingroup.clear();
							}

						}
						ii++;
					}
					root["result"]["MaxNoOfGroups"] = MaxNoOfGroups;

				}
			}
			root["status"] = "OK";
			root["title"] = "ZWaveGroupInfo";
		}

		void CWebServer::Cmd_ZWaveCancel(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->CancelControllerCommand();
				root["status"] = "OK";
				root["title"] = "ZWaveCancel";
			}
		}

		void CWebServer::Cmd_ApplyZWaveNodeConfig(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string svaluelist = m_pWebEm->FindValue("valuelist");
			if (
				(idx == "") ||
				(svaluelist == "")
				)
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = atoi(result[0][1].c_str());
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->ApplyNodeConfig(homeID, nodeID, svaluelist))
						return;
					root["status"] = "OK";
					root["title"] = "ApplyZWaveNodeConfig";
				}
			}
		}

		void CWebServer::Cmd_ZWaveRequestNodeConfig(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = atoi(result[0][1].c_str());
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RequestNodeConfig(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "RequestZWaveNodeConfig";
				}
			}
		}

		void CWebServer::Cmd_ZWaveReceiveConfigurationFromOtherController(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->ReceiveConfigurationFromOtherController();
				root["status"] = "OK";
				root["title"] = "ZWaveReceiveConfigurationFromOtherController";
			}
		}

		void CWebServer::Cmd_ZWaveSendConfigurationToSecondaryController(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->SendConfigurationToSecondaryController();
				root["status"] = "OK";
				root["title"] = "ZWaveSendConfigToSecond";
			}
		}

		void CWebServer::Cmd_ZWaveTransferPrimaryRole(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->TransferPrimaryRole();
				root["status"] = "OK";
				root["title"] = "ZWaveTransferPrimaryRole";
			}
		}
		std::string CWebServer::ZWaveGetConfigFile()
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return "";
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					std::string szConfigFile = "";
					m_retstr = pOZWHardware->GetConfigFile(szConfigFile);
					if (m_retstr != "")
					{
						m_pWebEm->m_outputfilename = szConfigFile;
					}
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPIndex()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->SetAllNodesChanged();
					std::string wwwFile = szWWWFolder + "/ozwcp/cp.html";
					std::ifstream testFile(wwwFile.c_str(), std::ios::binary);
					std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
						std::istreambuf_iterator<char>());
					if (fileContents.size() > 0)
					{
						m_retstr.insert(m_retstr.begin(), fileContents.begin(), fileContents.end());
					}
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPPollXml()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SendPollResponse();
					m_pWebEm->m_outputfilename = "poll.xml";
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeGetConf()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("node") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sNode = m_pWebEm->FindValue("node");
			if (sNode == "")
				return m_retstr;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SendNodeConfResponse(iNode);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeGetValues()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("node") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sNode = m_pWebEm->FindValue("node");
			if (sNode == "")
				return m_retstr;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SendNodeValuesResponse(iNode);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeSetValue()
		{
			m_retstr = "";
			std::vector<std::string> strarray;
			StringSplit(m_pWebEm->m_ActualRequest.content, "=", strarray);
			if (strarray.size() != 2)
				return "";

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SetNodeValue(strarray[0], strarray[1]);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeSetButton()
		{
			m_retstr = "";
			std::vector<std::string> strarray;
			StringSplit(m_pWebEm->m_ActualRequest.content, "=", strarray);
			if (strarray.size() != 2)
				return "";

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SetNodeButton(strarray[0], strarray[1]);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPAdminCommand()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("fun") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sFun = m_pWebEm->FindValue("fun");
			std::string sNode = m_pWebEm->FindValue("node");
			std::string sButton = m_pWebEm->FindValue("button");

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->DoAdminCommand(sFun, atoi(sNode.c_str()), atoi(sButton.c_str()));
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPNodeChange()
		{
			m_retstr = "";
			if (m_pWebEm->m_ActualRequest.content.find("fun") == std::string::npos)
				return "";
			m_pWebEm->MakeValuesFromPostContent(&m_pWebEm->m_ActualRequest);
			std::string sFun = m_pWebEm->FindValue("fun");
			std::string sNode = m_pWebEm->FindValue("node");
			std::string sValue = m_pWebEm->FindValue("value");

			if (sNode.size() > 4)
				sNode = sNode.substr(4);

			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->DoNodeChange(sFun, atoi(sNode.c_str()), sValue);
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPSaveConfig()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->SaveConfig();
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPGetTopo()
		{
			m_retstr = "";
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					m_retstr = pOZWHardware->GetCPTopo();
					m_pWebEm->m_outputfilename = "topo.xml";
				}
			}
			return m_retstr;
		}
		std::string CWebServer::ZWaveCPGetStats()
		{
			m_retstr = "";
			//Crashes at OpenZWave::GetNodeStatistics::_data->m_sentTS = m_sentTS.GetAsString();
			/*
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
			if (pHardware->HwdType == HTYPE_OpenZWave)
			{
			COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
			m_retstr = pOZWHardware->GetCPStats();
			m_pWebEm->m_outputfilename = "stats.xml";
			}
			}
			*/
			return m_retstr;
		}



		void CWebServer::Cmd_ZWaveSetUserCodeEnrollmentMode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
				root["status"] = "OK";
				root["title"] = "SetUserCodeEnrollmentMode";
				pOZWHardware->SetUserCodeEnrollmentMode();
			}
		}

		void CWebServer::Cmd_ZWaveRemoveUserCode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string scodeindex = m_pWebEm->FindValue("codeindex");
			if (
				(idx == "") ||
				(scodeindex == "")
				)
				return;
			int iCodeIndex = atoi(scodeindex.c_str());

			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = atoi(result[0][1].c_str());
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->RemoveUserCode(homeID, nodeID, iCodeIndex))
						return;
					root["status"] = "OK";
					root["title"] = "RemoveUserCode";
				}
			}
		}

		void CWebServer::Cmd_ZWaveGetNodeUserCodes(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;

			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);
			if (result.size() > 0)
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = atoi(result[0][1].c_str());
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave *pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->GetNodeUserCodes(homeID, nodeID, root))
						return;
					root["status"] = "OK";
					root["title"] = "GetUserCodes";
				}
			}
		}
	}
}

