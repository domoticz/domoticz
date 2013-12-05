// 1-wire by kernel support
// Linux modules w1-gpio and w1-therm have to be loaded (add they in /etc/modules or use modprobe)
// Others devices are accessed by default mode (no other module needed)
//
// Note : we add to do read/write operations in a thread because access to 'rw' file takes too much time (why ???)
//

#include "stdafx.h"
#include "1WireByKernel.h"

#include <fstream>
#include "../main/Logger.h"

#ifdef _DEBUG
   #ifdef WIN32
	   #define Wire1_Base_Dir "E:\\w1\\devices"
   #else // WIN32
	   #define Wire1_Base_Dir "/sys/bus/w1/devices"
   #endif // WIN32
#else // _DEBUG
	#define Wire1_Base_Dir "/sys/bus/w1/devices"
#endif //_DEBUG


C1WireByKernel::C1WireByKernel() :
   m_AllDevicesInitialized(false)
{
   m_Thread = new boost::thread(&C1WireByKernel::ThreadFunction,this);
}

C1WireByKernel::~C1WireByKernel()
{
   m_Thread->interrupt();
   m_Thread->join();
   delete m_Thread;
}

bool C1WireByKernel::IsAvailable()
{
   //Check if system have the w1-gpio interface
   std::ifstream infile1wire;
   std::string wire1catfile=Wire1_Base_Dir;
   wire1catfile+="/w1_bus_master1/w1_master_slaves";
   infile1wire.open(wire1catfile.c_str());
   if (infile1wire.is_open())
   {
      infile1wire.close();
      return true;
   }
   return false;
}

void C1WireByKernel::ThreadFunction()
{
   ThreadBuildDevicesList();
   try
   {
      while (1)
      {
         // Read state of all devices
         for(DeviceCollection::const_iterator it=m_Devices.begin();it!=m_Devices.end();++it)
         {
            // Next read one device state
            try
            {
               // Priority to changes asked by Domoticz
               ThreadProcessPendingChanges();

               DeviceState* device=(*it).second;
               switch(device->GetDevice().family)
               {
               case high_precision_digital_thermometer:
                  {
                     Locker l(m_Mutex);
                     device->m_Temperature=ThreadReadRawDataHighPrecisionDigitalThermometer(device->GetDevice().filename);
                     break;
                  }
               case dual_channel_addressable_switch:
                  {
                     unsigned char answer=ThreadReadRawDataDualChannelAddressableSwitch(device->GetDevice().filename);
                     // Don't update if change is pending
                     Locker l(m_Mutex);
                     if (!GetDevicePendingState(device->GetDevice().devid))
                     {
                        // Caution : 0 means 'transistor active', we have to invert
                        device->m_DigitalIo[0]=(answer&0x01)?false:true;
                        device->m_DigitalIo[1]=(answer&0x04)?false:true;
                     }
                     break;
                  }
               case _8_channel_addressable_switch:
                  {
                     unsigned char answer=ThreadReadRawData8ChannelAddressableSwitch(device->GetDevice().filename);
                     // Don't update if change is pending
                     Locker l(m_Mutex);
                     if (!GetDevicePendingState(device->GetDevice().devid))
                     {
                        for (unsigned int idxBit=0, mask=0x01;mask!=0x100;mask<<=1)
                        {
                           // Caution : 0 means 'transistor active', we have to invert
                           device->m_DigitalIo[idxBit++]=(answer&mask)?false:true;
                        }
                     }
                     break;
                  }
               default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
                  break;
               }
            }
            catch(const OneWireReadErrorException& e)
            {
               _log.Log(LOG_NORM,e.what());
            }
         }

         // Needed at startup to not report wrong states
         m_AllDevicesInitialized=true;

         // Wait only if no pending changes
         boost::mutex::scoped_lock lock(m_PendingChangesMutex);
         boost::system_time const timeout=boost::get_system_time()+boost::posix_time::seconds(10);
         m_PendingChangesCondition.timed_wait(lock,timeout,IsPendingChanges(m_PendingChanges));
      }
   }
   catch(boost::thread_interrupted&)
   {
      // Thread is stopped
   }
   m_PendingChanges.clear();
   for (DeviceCollection::iterator it=m_Devices.begin();it!=m_Devices.end();++it) {delete (*it).second;}
   m_Devices.clear();
}

void C1WireByKernel::ThreadProcessPendingChanges()
{
   while (!m_PendingChanges.empty())
   {
      bool success=false;
      int writeTries=3;
      do
      {
         try
         {
            DeviceState device=m_PendingChanges.front();

            switch(device.GetDevice().family)
            {
            case dual_channel_addressable_switch:
               {
                  ThreadWriteRawDataDualChannelAddressableSwitch(device.GetDevice().filename,device.m_Unit,device.m_DigitalIo[device.m_Unit]);
                  break;
               }
            case _8_channel_addressable_switch:
               {
                  ThreadWriteRawData8ChannelAddressableSwitch(device.GetDevice().filename,device.m_Unit,device.m_DigitalIo[device.m_Unit]);
                  break;
               }
            default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
               return;
            }
         }
         catch(const OneWireReadErrorException& e)
         {
            _log.Log(LOG_NORM,e.what());
            continue;
         }
         catch(const OneWireWriteErrorException& e)
         {
            _log.Log(LOG_NORM,e.what());
            continue;
         }
         success=true;
      }
      while(!success&&writeTries--);

      Locker l(m_Mutex);
      m_PendingChanges.pop_front();
   }
}

void C1WireByKernel::ThreadBuildDevicesList()
{
	std::string catfile=Wire1_Base_Dir;
	catfile+="/w1_bus_master1/w1_master_slaves";
	std::ifstream infile;
   std::string sLine;

   for (DeviceCollection::iterator it=m_Devices.begin();it!=m_Devices.end();++it) {delete (*it).second;}
   m_Devices.clear();

   infile.open(catfile.c_str());
	if (!infile.is_open())
      return;

   Locker l(m_Mutex);
   while (!infile.eof())
   {
      getline(infile, sLine);
      if (sLine.size()!=0)
      {
         // Get the device from it's name
         _t1WireDevice device;
         GetDevice(sLine, device);

         switch (device.family)
         {
         case high_precision_digital_thermometer:
         case dual_channel_addressable_switch:
         case _8_channel_addressable_switch:
            m_Devices[device.devid]=new DeviceState(device);
            break;
         default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
            break;
         }
      }
   }

   infile.close();
}

void C1WireByKernel::GetDevices(/*out*/std::vector<_t1WireDevice>& devices) const
{
   // If devices not initialized (ie : at least one time read), return an empty list
   // to not update Domoticz with inconsistent values
   if (!m_AllDevicesInitialized)
      return;

   Locker l(m_Mutex);
   for (DeviceCollection::const_iterator it=m_Devices.begin();it!=m_Devices.end();++it)
      devices.push_back(((*it).second)->GetDevice());
}

const C1WireByKernel::DeviceState* C1WireByKernel::GetDevicePendingState(const std::string& deviceId) const
{
   for (std::list<DeviceState>::const_reverse_iterator it=m_PendingChanges.rbegin();it!=m_PendingChanges.rend();++it)
   {
      if ((*it).GetDevice().devid==deviceId)
         return &(*it);
   }
   return NULL;
}

float C1WireByKernel::GetTemperature(const _t1WireDevice& device) const
{
   switch (device.family)
   {
      case high_precision_digital_thermometer:
         {
            DeviceCollection::const_iterator devIt=m_Devices.find(device.devid);
            return (devIt==m_Devices.end())?0.0f:(*devIt).second->m_Temperature;
         }
      default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
         return 0.0;
   }
}

float C1WireByKernel::GetHumidity(const _t1WireDevice& device) const
{
   return 0.0f;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

bool C1WireByKernel::GetLightState(const _t1WireDevice& device,int unit) const
{
   switch (device.family)
   {
      case dual_channel_addressable_switch:
      case _8_channel_addressable_switch:
         {
            // Return future state if change is pending
            Locker l(m_Mutex);
            const DeviceState* pendingState=GetDevicePendingState(device.devid);
            if (pendingState)
               return pendingState->m_DigitalIo[unit];

            // Else return current state
            DeviceCollection::const_iterator devIt=m_Devices.find(device.devid);
            return (devIt==m_Devices.end())?false:(*devIt).second->m_DigitalIo[unit];
         }
      default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
         return false;
   }
}

unsigned int C1WireByKernel::GetNbChannels(const _t1WireDevice& device) const
{
   return 0;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

unsigned long C1WireByKernel::GetCounter(const _t1WireDevice& device,int unit) const
{
   return 0;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

int C1WireByKernel::GetVoltage(const _t1WireDevice& device,int unit) const
{
   return 0;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

float C1WireByKernel::ThreadReadRawDataHighPrecisionDigitalThermometer(const std::string& deviceFileName) const
{
   // The only native-supported device (by w1-therm module)
   std::string data;
   bool bFoundCrcOk=false;

   static const std::string tag("t=");

   std::ifstream file;
   std::string fileName=deviceFileName;
   fileName+="/w1_slave";
   file.open(fileName.c_str());
   if (file.is_open())
   {
      std::string sLine;
      while (!file.eof())
      {
         getline(file, sLine);
         int tpos;
         if (sLine.find("crc=")!=std::string::npos)
         {
            if (sLine.find("YES")!=std::string::npos)
               bFoundCrcOk=true;
         }
         else if ((tpos=sLine.find(tag))!=std::string::npos)
         {
            data = sLine.substr(tpos+tag.length());
         }
      }
      file.close();
   }

   if (bFoundCrcOk)
      return (float)atoi(data.c_str())/1000.0f; // Temperature given by kernel is in thousandths of degrees

   throw OneWireReadErrorException(deviceFileName);
}

bool C1WireByKernel::sendAndReceiveByRwFile(std::string path,const unsigned char * const cmd,size_t cmdSize,unsigned char * const answer,size_t answerSize) const
{
   bool ok = false;
   bool pioState = false;
   std::fstream file;
   path+="/rw";
   file.open(path.c_str(),std::ios_base::in|std::ios_base::out|std::ios_base::binary);
   if (file.is_open())
   {
      if (file.write((char*)cmd,cmdSize))
      {
         if (file.read((char*)answer,answerSize))
            ok = true;
      }
      file.close();
   }

   return ok;
}

unsigned char C1WireByKernel::ThreadReadRawDataDualChannelAddressableSwitch(const std::string& deviceFileName) const
{
   static const unsigned char CmdReadPioRegisters[]={0xF5};
   unsigned char answer[1];
   if (!sendAndReceiveByRwFile(deviceFileName,CmdReadPioRegisters,sizeof(CmdReadPioRegisters),answer,sizeof(answer)))
      throw OneWireReadErrorException(deviceFileName);
   // Check received data : datasheet says that b[7-4] is complement to b[3-0]
   if ((answer[0]&0xF0)!=(((~answer[0])&0x0F)<<4))
      throw OneWireReadErrorException(deviceFileName);
   return answer[0];
}

unsigned char C1WireByKernel::ThreadReadRawData8ChannelAddressableSwitch(const std::string& deviceFileName) const
{
   static const unsigned char CmdReadPioRegisters[]={0xF5};
   unsigned char answer[33];
   if (!sendAndReceiveByRwFile(deviceFileName,CmdReadPioRegisters,sizeof(CmdReadPioRegisters),answer,sizeof(answer)))
      throw OneWireReadErrorException(deviceFileName);

   // Now check the Crc
   unsigned char crcData[33];
   crcData[0]=CmdReadPioRegisters[0];
   for(int i=0;i<32;i++)
      crcData[i+1]=answer[i];
   if (Crc16(crcData,sizeof(crcData))!=answer[32])
      throw OneWireReadErrorException(deviceFileName);

   return answer[0];
}

void C1WireByKernel::SetLightState(const std::string& sId,int unit,bool value)
{
   DeviceCollection::const_iterator it=m_Devices.find(sId);
   if (it==m_Devices.end())
      return;

   _t1WireDevice device=(*it).second->GetDevice();

   switch(device.family)
   {
   case dual_channel_addressable_switch:
      {
         if (unit<0 || unit>1)
            return;
         DeviceState deviceState(device);
         deviceState.m_Unit=unit;
         deviceState.m_DigitalIo[unit]=value;
         Locker l(m_Mutex);
         m_Devices[device.devid]->m_DigitalIo[unit]=value;
         m_PendingChanges.push_back(deviceState);
         break;
      }
   case _8_channel_addressable_switch:
      {
         if (unit<0 || unit>7)
            return;
         DeviceState deviceState(device);
         deviceState.m_Unit=unit;
         deviceState.m_DigitalIo[unit]=value;
         Locker l(m_Mutex);
         m_Devices[device.devid]->m_DigitalIo[unit]=value;
         m_PendingChanges.push_back(deviceState);
         break;
      }
   default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
      return;
   }
}

void C1WireByKernel::ThreadWriteRawDataDualChannelAddressableSwitch(const std::string& deviceFileName,int unit,bool value) const
{
   // First read actual state, to only change the correct unit
   unsigned char readAnswer=ThreadReadRawDataDualChannelAddressableSwitch(deviceFileName);
   // Caution : 0 means 'transistor active', we have to invert
   unsigned char otherPioState=0;
   otherPioState|=(readAnswer&0x01)?0x00:0x01;
   otherPioState|=(readAnswer&0x04)?0x00:0x02;

   unsigned char CmdWritePioRegisters[]={0x5A,0x00,0x00};
   // 0 to activate the transistor
   unsigned char newPioState = value ? (otherPioState | (0x01<<unit)) : (otherPioState & ~(0x01<<unit));
   CmdWritePioRegisters[1] = ~newPioState;
   CmdWritePioRegisters[2] = ~CmdWritePioRegisters[1];

   unsigned char answer[2];
   sendAndReceiveByRwFile(deviceFileName,CmdWritePioRegisters,sizeof(CmdWritePioRegisters),answer,sizeof(answer));

   // Check for transmission errors
   // Expected first byte = 0xAA
   // Expected second Byte = b[7-4] is complement to b[3-0]
   if (answer[0]!=0xAA || (answer[1]&0xF0)!=(((~answer[1])&0x0F)<<4))
      throw OneWireWriteErrorException(deviceFileName);
}

void C1WireByKernel::ThreadWriteRawData8ChannelAddressableSwitch(const std::string& deviceFileName,int unit,bool value) const
{
   // First read actual state, to only change the correct unit
   // Caution : 0 means 'transistor active', we have to invert
   unsigned char otherPioState = ~ThreadReadRawData8ChannelAddressableSwitch(deviceFileName);

   unsigned char CmdWritePioRegisters[]={0x5A,0x00,0x00};
   // 0 to activate the transistor
   unsigned char newPioState = value ? (otherPioState | (0x01<<unit)) : (otherPioState & ~(0x01<<unit));
   CmdWritePioRegisters[1] = ~newPioState;
   CmdWritePioRegisters[2] = ~CmdWritePioRegisters[1];

   unsigned char answer[2];
   sendAndReceiveByRwFile(deviceFileName,CmdWritePioRegisters,sizeof(CmdWritePioRegisters),answer,sizeof(answer));

   // Check for transmission errors
   // Expected first byte = 0xAA
   // Expected second Byte = inverted new pio state
   if (answer[0]!=0xAA || answer[1]!=(unsigned char)(~newPioState))
      throw OneWireWriteErrorException(deviceFileName);
}

inline void std_to_upper(const std::string& str, std::string& converted)
{
	converted="";
	for(size_t i = 0; i < str.size(); ++i)
		converted += toupper(str[i]);
}

void C1WireByKernel::GetDevice(const std::string& deviceName, /*out*/_t1WireDevice& device) const
{
   // 1W-Kernel device name format : ff-iiiiiiiiiiii, with :
   // - ff : family (1 byte)
   // - iiiiiiiiiiii : id (6 bytes)

   // Retrieve family from the first 2 chars
   device.family=ToFamily(deviceName.substr(0,2));

   // Device Id (6 chars after '-'), upper case
   std_to_upper(deviceName.substr(3,3+6*2),device.devid);

   // Filename (full path)
   device.filename=Wire1_Base_Dir;
   device.filename+="/" + deviceName;
}

