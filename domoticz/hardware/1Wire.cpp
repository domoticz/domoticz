
#include "stdafx.h"
#include "1Wire.h"
#include "1Wire/1WireByOWFS.h"
#include "1Wire/1WireByKernel.h"
#ifdef WIN32
#include "1Wire/1WireForWindows.h"
#endif // WIN32
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _DEBUG
#define Wire1_POLL_INTERVAL 5
#else // _DEBUG
#define Wire1_POLL_INTERVAL 30
#endif //_DEBUG

#define round(a) ( int ) ( a + .5 )

C1Wire::C1Wire(const int ID) :
	m_stoprequested(false),
   m_system(NULL)
{
   m_HwdID=ID;
	DetectSystem();
}

C1Wire::~C1Wire()
{
}

bool C1Wire::Have1WireSystem()
{
   LogSystem();

#ifdef WIN32
	return (C1WireForWindows::IsAvailable());
#else // WIN32
   return (C1WireByOWFS::IsAvailable()||C1WireByKernel::IsAvailable());
#endif // WIN32
}

void C1Wire::DetectSystem()
{
   LogSystem();

#ifdef WIN32
   if (!m_system && C1WireForWindows::IsAvailable())
      m_system=new C1WireForWindows();
#else // WIN32

   // Using the both systems at same time results in conflicts,
   // see http://owfs.org/index.php?page=w1-project.
   // So priority is given to OWFS (more powerfull than kernel)
   if (C1WireByOWFS::IsAvailable()) {
      m_system=new C1WireByOWFS();
	_log.Log(LOG_NORM,"1-Wire: Using OWFS...");
   }
   else if (C1WireByKernel::IsAvailable()) {
      m_system=new C1WireByKernel();
	_log.Log(LOG_NORM,"1-Wire: Using Kernel...");
   }

#endif // WIN32
}

void C1Wire::LogSystem()
{
   static bool alreadyLogged=false;
   if (alreadyLogged)
      return;
   alreadyLogged=true;

#ifdef WIN32
   if (C1WireForWindows::IsAvailable())
      { _log.Log(LOG_NORM,"1-Wire support available..."); return; }
#else // WIN32

   if (C1WireByOWFS::IsAvailable())
      { _log.Log(LOG_NORM,"1-Wire support available (By OWFS)..."); return; }
   if (C1WireByKernel::IsAvailable())
      { _log.Log(LOG_NORM,"1-Wire support available (By Kernel)..."); return; }

#endif // WIN32
}

bool C1Wire::StartHardware()
{
	// Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&C1Wire::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool C1Wire::StopHardware()
{
   if (m_thread)
   {
      m_stoprequested = true;
      m_thread->join();
   }
   m_bIsStarted=false;
   if (m_system)
   {
      delete m_system;
      m_system=NULL;
   }

   return true;
}

void C1Wire::Do_Work()
{
 	time_t lastPollTime=mytime(NULL);
   GetDeviceDetails();
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (mytime(NULL)-lastPollTime>=Wire1_POLL_INTERVAL)
		{
         GetDeviceDetails();
			lastPollTime=mytime(NULL);
		}
	}
}

void C1Wire::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pSen=(tRBUF*)pdata;

   if (!m_system)
      return;//no 1-wire support

   if (pSen->ICMND.packettype==pTypeLighting2 && pSen->LIGHTING2.subtype==sTypeAC)
	{
		//light command

      unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
      deviceIdByteArray[0]=pSen->LIGHTING2.id1;
      deviceIdByteArray[1]=pSen->LIGHTING2.id2;
      deviceIdByteArray[2]=pSen->LIGHTING2.id3;
      deviceIdByteArray[3]=pSen->LIGHTING2.id4;

      _t1WireDevice device;
      m_system->SetLightState(ByteArrayToDeviceId(deviceIdByteArray),pSen->LIGHTING2.unitcode,pSen->LIGHTING2.cmnd==light2_sOn);
	}
}

bool IsTemperatureValid(_e1WireFamilyType deviceFamily, float temperature)
{
   if (temperature<=-300 || temperature>=381)
      return false;

   // Some devices has a power-on value at 85°, we have to filter it
   switch (deviceFamily)
   {
      case high_precision_digital_thermometer:
      case Econo_Digital_Thermometer:
      case programmable_resolution_digital_thermometer:
      case Temperature_memory:
      case Temperature_IO:
         if (temperature == 85)
            return false;
   }

   return true;
}

void C1Wire::GetDeviceDetails()
{
   if (!m_system)
      return;

   // Get all devices
	std::vector<_t1WireDevice> devices;
   m_system->GetDevices(devices);

	// Parse our devices (have to test m_stoprequested because it can take some time in case of big networks)
	std::vector<_t1WireDevice>::const_iterator itt;
	for (itt=devices.begin(); itt!=devices.end() && !m_stoprequested; ++itt)
	{
      const _t1WireDevice& device=*itt;

      // Manage families specificities
      switch(device.family)
      {
      case high_precision_digital_thermometer:
      case Thermachron:
      case Econo_Digital_Thermometer:
      case Temperature_memory:
      case programmable_resolution_digital_thermometer:
         {
            float temperature=m_system->GetTemperature(device);

            // Filter invalid temperature
            if (IsTemperatureValid(device.family,temperature))
               ReportTemperature(device.devid,temperature);

            break;
         }

      case Addresable_Switch:
      case microlan_coupler:
         {
            ReportLightState(device.devid,0,m_system->GetLightState(device,0));
            break;
         }

      case dual_addressable_switch_plus_1k_memory:
         {
            ReportLightState(device.devid,0,m_system->GetLightState(device,0));
            if (m_system->GetNbChannels(device) == 2)
               ReportLightState(device.devid,1,m_system->GetLightState(device,1));
            break;
         }

      case _8_channel_addressable_switch:
         {
            ReportLightState(device.devid,0,m_system->GetLightState(device,0));
            ReportLightState(device.devid,1,m_system->GetLightState(device,1));
            ReportLightState(device.devid,2,m_system->GetLightState(device,2));
            ReportLightState(device.devid,3,m_system->GetLightState(device,3));
            ReportLightState(device.devid,4,m_system->GetLightState(device,4));
            ReportLightState(device.devid,5,m_system->GetLightState(device,5));
            ReportLightState(device.devid,6,m_system->GetLightState(device,6));
            ReportLightState(device.devid,7,m_system->GetLightState(device,7));
            break;
         }

      case Temperature_IO:
         {
            ReportTemperature(device.devid,m_system->GetTemperature(device));
            ReportLightState(device.devid,0,m_system->GetLightState(device,0));
            ReportLightState(device.devid,1,m_system->GetLightState(device,1));
            break;
         }

      case dual_channel_addressable_switch:
      case _4k_EEPROM_with_PIO:
         {
            ReportLightState(device.devid,0,m_system->GetLightState(device,0));
            ReportLightState(device.devid,1,m_system->GetLightState(device,1));
            break;
         }

      case Environmental_Monitors:
         {
            ReportTemperatureHumidity(device.devid,m_system->GetTemperature(device),m_system->GetHumidity(device));
            break;
         }

      case _4k_ram_with_counter:
         {
            ReportCounter(device.devid,0,m_system->GetCounter(device,0));
            ReportCounter(device.devid,1,m_system->GetCounter(device,1));
            break;
         }

      case quad_ad_converter:
         {
            ReportVoltage(0,m_system->GetVoltage(device,0));
            ReportVoltage(1,m_system->GetVoltage(device,1));
            ReportVoltage(2,m_system->GetVoltage(device,2));
            ReportVoltage(3,m_system->GetVoltage(device,3));
            break;
         }

      case Serial_ID_Button:
         {
            // Nothing to do with these devices for Domoticz ==> Filtered
            break;
         }

      default: // Device is not actually supported
         {
            _log.Log(LOG_NORM,"1-Wire : Device family (%02x) is not actualy supported", device.family);
            break;
         }
      }
	}
}

void C1Wire::ReportTemperature(const std::string& deviceId,float temperature)
{
   unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
   DeviceIdToByteArray(deviceId,deviceIdByteArray);

   RBUF tsen;
   memset(&tsen,0,sizeof(RBUF));
   tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
   tsen.TEMP.packettype=pTypeTEMP;
   tsen.TEMP.subtype=sTypeTEMP10;
   tsen.TEMP.battery_level=9;
   tsen.TEMP.rssi=12;
   tsen.TEMP.id1=(BYTE)deviceIdByteArray[0];
   tsen.TEMP.id2=(BYTE)deviceIdByteArray[1];

   tsen.TEMP.tempsign=(temperature>=0)?0:1;
   int at10=round(abs(temperature*10.0f));
   tsen.TEMP.temperatureh=(BYTE)(at10/256);
   at10-=(tsen.TEMP.temperatureh*256);
   tsen.TEMP.temperaturel=(BYTE)(at10);

   sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
}

void C1Wire::ReportHumidity(const std::string& deviceId,float humidity)
{
   unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
   DeviceIdToByteArray(deviceId,deviceIdByteArray);

   RBUF tsen;
   memset(&tsen,0,sizeof(RBUF));
   tsen.HUM.packetlength=sizeof(tsen.HUM)-1;
   tsen.HUM.packettype=pTypeHUM;
   tsen.HUM.subtype=sTypeHUM2;
   tsen.HUM.battery_level=9;
   tsen.HUM.rssi=12;
   tsen.TEMP.id1=(BYTE)deviceIdByteArray[0];
   tsen.TEMP.id2=(BYTE)deviceIdByteArray[1];

   tsen.HUM.humidity=(BYTE)round(humidity);
   tsen.HUM.humidity_status=Get_Humidity_Level(tsen.HUM.humidity);

   sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM);//decode message
}

void C1Wire::ReportTemperatureHumidity(const std::string& deviceId,float temperature,float humidity)
{
   unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
   DeviceIdToByteArray(deviceId,deviceIdByteArray);

   RBUF tsen;
   memset(&tsen,0,sizeof(RBUF));
   tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
   tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
   tsen.TEMP_HUM.subtype=sTypeTH5;
   tsen.TEMP_HUM.battery_level=9;
   tsen.TEMP_HUM.rssi=12;
   tsen.TEMP.id1=(BYTE)deviceIdByteArray[0];
   tsen.TEMP.id2=(BYTE)deviceIdByteArray[1];

   tsen.TEMP_HUM.tempsign=(temperature>=0)?0:1;
   int at10=round(abs(temperature*10.0f));
   tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
   at10-=(tsen.TEMP_HUM.temperatureh*256);
   tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
   tsen.TEMP_HUM.humidity=(BYTE)round(humidity);
   tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

   sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);//decode message
}

void C1Wire::ReportLightState(const std::string& deviceId,int unit,bool state)
{
   unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
   DeviceIdToByteArray(deviceId,deviceIdByteArray);

   RBUF tsen;
   memset(&tsen,0,sizeof(RBUF));
   tsen.LIGHTING2.packetlength=sizeof(tsen.LIGHTING2)-1;
   tsen.LIGHTING2.packettype=pTypeLighting2;
   tsen.LIGHTING2.subtype=sTypeAC;
   tsen.LIGHTING2.seqnbr=0;
   tsen.LIGHTING2.id1=(BYTE)deviceIdByteArray[0];
   tsen.LIGHTING2.id2=(BYTE)deviceIdByteArray[1];
   tsen.LIGHTING2.id3=(BYTE)deviceIdByteArray[2];
   tsen.LIGHTING2.id4=(BYTE)deviceIdByteArray[3];
   tsen.LIGHTING2.unitcode=unit;
   tsen.LIGHTING2.cmnd=state?light2_sOn:light2_sOff;
   tsen.LIGHTING2.level=0;
   tsen.LIGHTING2.rssi=12;
   sDecodeRXMessage(this, (const unsigned char *)&tsen.LIGHTING2);//decode message
}

void C1Wire::ReportCounter(const std::string& deviceId,int unit,unsigned long counter)
{
   unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
   DeviceIdToByteArray(deviceId,deviceIdByteArray);

   RBUF tsen;
   memset(&tsen,0,sizeof(RBUF));
   tsen.RFXMETER.packetlength=sizeof(tsen.RFXMETER)-1;
   tsen.RFXMETER.packettype=pTypeRFXMeter;
   tsen.RFXMETER.subtype=sTypeRFXMeterCount;
   tsen.RFXMETER.rssi=12;
   tsen.RFXMETER.id1=(BYTE)deviceIdByteArray[0];
   tsen.RFXMETER.id2=(BYTE)deviceIdByteArray[1] + unit;

   tsen.RFXMETER.count1 = (BYTE)((counter & 0xFF000000) >> 24);
   tsen.RFXMETER.count2 = (BYTE)((counter & 0x00FF0000) >> 16);
   tsen.RFXMETER.count3 = (BYTE)((counter & 0x0000FF00) >> 8);
   tsen.RFXMETER.count4 = (BYTE)(counter & 0x000000FF);
   sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER);//decode message
}

void C1Wire::ReportVoltage(int unit,int voltage)
{
   RBUF tsen;
   memset(&tsen,0,sizeof(RBUF));
   tsen.RFXSENSOR.packetlength=sizeof(tsen.RFXSENSOR)-1;
   tsen.RFXSENSOR.packettype=pTypeRFXSensor;
   tsen.RFXSENSOR.subtype=sTypeRFXSensorVolt;
   tsen.RFXSENSOR.rssi=12;
   tsen.RFXSENSOR.id=unit+1;

   tsen.RFXSENSOR.msg1 = (BYTE)(voltage/256);
   tsen.RFXSENSOR.msg2 = (BYTE)(voltage-(tsen.RFXSENSOR.msg1*256));
   sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXSENSOR);//decode message
}
