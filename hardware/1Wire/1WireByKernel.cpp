// 1-wire by kernel support
// Linux modules w1-gpio and w1-therm have to be loaded (add they in /etc/modules or use modprobe)
// Others devices are accessed by default mode (no other module needed)
//
// Note : we add to do read/write operations in a thread because access to 'rw' file takes too much time (why ???)
//

#include "stdafx.h"
#include "1WireByKernel.h"

#include <fstream>
#include "../../main/Logger.h"
#include "../../main/Helper.h"
#include "../../main/mainworker.h"

#ifdef WIN32
	#include "../../main/dirent_windows.h"
#else
	#include <dirent.h>
#endif

#ifdef _DEBUG
	#ifdef WIN32
		#define Wire1_Base_Dir "E:\\w1\\devices"
	#else // WIN32
		#define Wire1_Base_Dir "/sys/bus/w1/devices"
	#endif // WIN32
#else // _DEBUG
	#define Wire1_Base_Dir "/sys/bus/w1/devices"
#endif //_DEBUG

C1WireByKernel::C1WireByKernel()
{
	m_thread = std::make_shared<std::thread>(&C1WireByKernel::ThreadFunction, this);
	SetThreadName(m_thread->native_handle(), "1WireByKernel");
	_log.Log(LOG_STATUS, "Using 1-Wire support (kernel W1 module)...");
}

C1WireByKernel::~C1WireByKernel()
{
	RequestStop();
	_t1WireDevice device;
	DeviceState deviceState(device);
	m_PendingChanges.push_back(deviceState); // wake up thread with dummy item
	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}
}

bool C1WireByKernel::IsAvailable()
{
	//Check if system have the w1-gpio interface
	std::ifstream infile1wire;
	std::string wire1catfile = Wire1_Base_Dir;
	wire1catfile += "/w1_bus_master1/w1_master_slaves";
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
	try
	{
		while (!IsStopRequested(0))
		{
			// Read state of all devices
			ReadStates();

			// Wait only if no pending changes
			std::unique_lock<std::mutex> lock(m_PendingChangesMutex);
			m_PendingChangesCondition.wait_for(lock, std::chrono::duration<int>(10), IsPendingChanges(m_PendingChanges));
		}
	}
	catch (...)
	{
		// Thread is stopped
	}
	m_PendingChanges.clear();
	for (auto &m_Device : m_Devices)
		delete m_Device.second;

	m_Devices.clear();
}

void C1WireByKernel::ReadStates()
{
	for (auto & it : m_Devices)
	{
		// Next read one device state
		try
		{
			// Priority to changes asked by Domoticz
			ThreadProcessPendingChanges();

			DeviceState* device = it.second;

			switch (device->GetDevice().family)
			{
			case high_precision_digital_thermometer:
			case programmable_resolution_digital_thermometer:
			{
				Locker l(m_Mutex);
				device->m_Temperature = ThreadReadRawDataHighPrecisionDigitalThermometer(device->GetDevice().filename);
				break;
			}
			case dual_channel_addressable_switch:
			{
				unsigned char answer = ThreadReadRawDataDualChannelAddressableSwitch(device->GetDevice().filename);
				// Don't update if change is pending
				Locker l(m_Mutex);
				if (!GetDevicePendingState(device->GetDevice().devid))
				{
					// Caution : 0 means 'transistor active', we have to invert
					device->m_DigitalIo[0] = (answer & 0x01) ? false : true;
					device->m_DigitalIo[1] = (answer & 0x04) ? false : true;
				}
				break;
			}
			case _8_channel_addressable_switch:
			{
				unsigned char answer = ThreadReadRawData8ChannelAddressableSwitch(device->GetDevice().filename);
				// Don't update if change is pending
				Locker l(m_Mutex);
				if (!GetDevicePendingState(device->GetDevice().devid))
				{
					for (unsigned int idxBit = 0, mask = 0x01; mask != 0x100; mask <<= 1)
					{
						// Caution : 0 means 'transistor active', we have to invert
						device->m_DigitalIo[idxBit++] = (answer&mask) ? false : true;
					}
				}
				break;
			}
			default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
				break;
			}
		}
		catch (const OneWireReadErrorException& e)
		{
			_log.Log(LOG_ERROR, "%s", e.what());
		}
	}

}

void C1WireByKernel::ThreadProcessPendingChanges()
{
	while (!m_PendingChanges.empty())
	{
		bool success = false;
		int writeTries = 3;
		do
		{
			try
			{
				DeviceState device = m_PendingChanges.front();

				switch (device.GetDevice().family)
				{
				case dual_channel_addressable_switch:
				{
					ThreadWriteRawDataDualChannelAddressableSwitch(device.GetDevice().filename, device.m_Unit, device.m_DigitalIo[device.m_Unit]);
					break;
				}
				case _8_channel_addressable_switch:
				{
					ThreadWriteRawData8ChannelAddressableSwitch(device.GetDevice().filename, device.m_Unit, device.m_DigitalIo[device.m_Unit]);
					break;
				}
				default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
					return;
				}
			}
			catch (const OneWireReadErrorException& e)
			{
				_log.Log(LOG_ERROR, "%s", e.what());
				continue;
			}
			catch (const OneWireWriteErrorException& e)
			{
				_log.Log(LOG_ERROR, "%s", e.what());
				continue;
			}
			success = true;
		} while (!success&&writeTries--);

		Locker l(m_Mutex);
		m_PendingChanges.pop_front();
	}
}

void C1WireByKernel::ThreadBuildDevicesList()
{
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(Wire1_Base_Dir)) != nullptr)
	{
		for (auto &m_Device : m_Devices)
			delete m_Device.second;

		m_Devices.clear();
		while ((ent = readdir(dir)) != nullptr)
		{
			std::string directoryName=ent->d_name;
			if(directoryName.find("w1_bus_master")==0)
			{

				std::string catfile=Wire1_Base_Dir;
				std::ifstream infile;
				std::string sLine;

				catfile+="/"+directoryName+"/w1_master_slaves";
				
				infile.open(catfile.c_str());
				if (!infile.is_open())
					return;

				Locker l(m_Mutex);
				while (!infile.eof())
				{
					getline(infile, sLine);
					if (!sLine.empty())
					{
						// Get the device from it's name
						_t1WireDevice device;
						GetDevice(sLine, device);
			
						switch (device.family)
						{
						case high_precision_digital_thermometer:
						case dual_channel_addressable_switch:
						case _8_channel_addressable_switch:
						case programmable_resolution_digital_thermometer:
							m_Devices[device.devid] = new DeviceState(device);
							_log.Log(LOG_STATUS, "1Wire: Added Device: %s", sLine.c_str());
							break;
						default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
							_log.Log(LOG_ERROR, "1Wire: Device not yet supported in Kernel mode (Please report!) ID:%s, family: %02X", sLine.c_str(), device.family);
							break;
						}
					}
				}
			
				infile.close();
			}
		}
		closedir (dir);
	}
}

void C1WireByKernel::GetDevices(/*out*/std::vector<_t1WireDevice>& devices) const
{
	Locker l(m_Mutex);
	std::transform(m_Devices.begin(), m_Devices.end(), std::back_inserter(devices),
		       [](const std::pair<const std::string, C1WireByKernel::DeviceState *> &m) { return m.second->GetDevice(); });
}

const C1WireByKernel::DeviceState* C1WireByKernel::GetDevicePendingState(const std::string& deviceId) const
{
	for (auto & it : m_PendingChanges)
	{
		if (it.GetDevice().devid == deviceId)
			return &it;
	}
	return nullptr;
}

float C1WireByKernel::GetTemperature(const _t1WireDevice& device) const
{
	switch (device.family)
	{
	case high_precision_digital_thermometer:
	case programmable_resolution_digital_thermometer:
	{
		DeviceCollection::const_iterator devIt = m_Devices.find(device.devid);
		return (devIt == m_Devices.end()) ? -1000.0f : (*devIt).second->m_Temperature;
	}
	default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
		return -1000.0;
	}
}

int C1WireByKernel::GetWiper(const _t1WireDevice& /*device*/) const
{
	return -1;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

float C1WireByKernel::GetHumidity(const _t1WireDevice& /*device*/) const
{
	return 0.0f;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

float C1WireByKernel::GetPressure(const _t1WireDevice& /*device*/) const
{
	return 0.0f;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

bool C1WireByKernel::GetLightState(const _t1WireDevice& device, int unit) const
{
	switch (device.family)
	{
	case dual_channel_addressable_switch:
	case _8_channel_addressable_switch:
	{
		// Return future state if change is pending
		Locker l(m_Mutex);
		const DeviceState* pendingState = GetDevicePendingState(device.devid);
		if (pendingState)
			return pendingState->m_DigitalIo[unit];

		// Else return current state
		DeviceCollection::const_iterator devIt = m_Devices.find(device.devid);
		return (devIt == m_Devices.end()) ? false : (*devIt).second->m_DigitalIo[unit];
	}
	default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
		return false;
	}
}

unsigned int C1WireByKernel::GetNbChannels(const _t1WireDevice& /*device*/) const
{
	return 0;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

unsigned long C1WireByKernel::GetCounter(const _t1WireDevice& /*device*/, int /*unit*/) const
{
	return 0;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

int C1WireByKernel::GetVoltage(const _t1WireDevice& /*device*/, int /*unit*/) const
{
	return -1000;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

float C1WireByKernel::GetIlluminance(const _t1WireDevice& /*device*/) const
{
	return -1000.0;// Device not supported in kernel mode (maybe later...), use OWFS solution.
}

void C1WireByKernel::StartSimultaneousTemperatureRead()
{
}

void C1WireByKernel::PrepareDevices()
{
	ThreadBuildDevicesList();
	ReadStates();
}

float C1WireByKernel::ThreadReadRawDataHighPrecisionDigitalThermometer(const std::string& deviceFileName) const
{
	// The only native-supported device (by w1-therm module)
	std::string data;
	bool bFoundCrcOk = false;

	static const std::string tag("t=");

	std::ifstream file;
	std::string fileName = deviceFileName;
	fileName += "/w1_slave";
	file.open(fileName.c_str());
	if (file.is_open())
	{
		std::string sLine;
		while (!file.eof())
		{
			getline(file, sLine);
			size_t tpos;
			if (sLine.find("crc=") != std::string::npos)
			{
				if (sLine.find("YES") != std::string::npos)
					bFoundCrcOk = true;
			}
			else if ((tpos = sLine.find(tag)) != std::string::npos)
			{
				data = sLine.substr(tpos + tag.length());
			}
		}
		file.close();
	}

	if (bFoundCrcOk)
	{
		if (is_number(data))
			return (float)atoi(data.c_str()) / 1000.0f; // Temperature given by kernel is in thousandths of degrees
	}

	throw OneWireReadErrorException(deviceFileName);
}

bool C1WireByKernel::sendAndReceiveByRwFile(std::string path, const unsigned char * const cmd, size_t cmdSize, unsigned char * const answer, size_t answerSize) const
{
	bool ok = false;
	//bool pioState = false;
	std::fstream file;
	path += "/rw";
	file.open(path.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
	if (file.is_open())
	{
		if (file.write((char*)cmd, cmdSize))
		{
			if (file.read((char*)answer, answerSize))
				ok = true;
		}
		file.close();
	}

	return ok;
}

unsigned char C1WireByKernel::ThreadReadRawDataDualChannelAddressableSwitch(const std::string& deviceFileName) const
{
	static const unsigned char CmdReadPioRegisters[] = { 0xF5 };
	unsigned char answer[1];
	if (!sendAndReceiveByRwFile(deviceFileName, CmdReadPioRegisters, sizeof(CmdReadPioRegisters), answer, sizeof(answer)))
		throw OneWireReadErrorException(deviceFileName);
	// Check received data : datasheet says that b[7-4] is complement to b[3-0]
	if ((answer[0] & 0xF0) != (((~answer[0]) & 0x0F) << 4))
		throw OneWireReadErrorException(deviceFileName);
	return answer[0];
}

unsigned char C1WireByKernel::ThreadReadRawData8ChannelAddressableSwitch(const std::string& deviceFileName) const
{
	static const unsigned char CmdReadPioRegisters[] = { 0xF5 };
	unsigned char answer[33];
	if (!sendAndReceiveByRwFile(deviceFileName, CmdReadPioRegisters, sizeof(CmdReadPioRegisters), answer, sizeof(answer)))
		throw OneWireReadErrorException(deviceFileName);

	// Now check the Crc
	unsigned char crcData[33];
	crcData[0] = CmdReadPioRegisters[0];
	for (int i = 0; i < 32; i++)
		crcData[i + 1] = answer[i];
	if (Crc16(crcData, sizeof(crcData)) != answer[32])
		throw OneWireReadErrorException(deviceFileName);

	return answer[0];
}

void C1WireByKernel::SetLightState(const std::string& sId, int unit, bool value, const unsigned int /*level*/)
{
	DeviceCollection::const_iterator it = m_Devices.find(sId);
	if (it == m_Devices.end())
		return;

	_t1WireDevice device = (*it).second->GetDevice();

	switch (device.family)
	{
	case dual_channel_addressable_switch:
	{
		if (unit < 0 || unit>1)
			return;
		DeviceState deviceState(device);
		deviceState.m_Unit = unit;
		deviceState.m_DigitalIo[unit] = value;
		Locker l(m_Mutex);
		m_Devices[device.devid]->m_DigitalIo[unit] = value;
		m_PendingChanges.push_back(deviceState);
		break;
	}
	case _8_channel_addressable_switch:
	{
		if (unit < 0 || unit>7)
			return;
		DeviceState deviceState(device);
		deviceState.m_Unit = unit;
		deviceState.m_DigitalIo[unit] = value;
		Locker l(m_Mutex);
		m_Devices[device.devid]->m_DigitalIo[unit] = value;
		m_PendingChanges.push_back(deviceState);
		break;
	}
	default: // Device not supported in kernel mode (maybe later...), use OWFS solution.
		return;
	}
}

void C1WireByKernel::ThreadWriteRawDataDualChannelAddressableSwitch(const std::string& deviceFileName, int unit, bool value) const
{
	// First read actual state, to only change the correct unit
	unsigned char readAnswer = ThreadReadRawDataDualChannelAddressableSwitch(deviceFileName);
	// Caution : 0 means 'transistor active', we have to invert
	unsigned char otherPioState = 0;
	otherPioState |= (readAnswer & 0x01) ? 0x00 : 0x01;
	otherPioState |= (readAnswer & 0x04) ? 0x00 : 0x02;

	unsigned char CmdWritePioRegisters[] = { 0x5A,0x00,0x00 };
	// 0 to activate the transistor
	unsigned char newPioState = value ? (otherPioState | (0x01 << unit)) : (otherPioState & ~(0x01 << unit));
	CmdWritePioRegisters[1] = ~newPioState;
	CmdWritePioRegisters[2] = ~CmdWritePioRegisters[1];

	unsigned char answer[2];
	sendAndReceiveByRwFile(deviceFileName, CmdWritePioRegisters, sizeof(CmdWritePioRegisters), answer, sizeof(answer));

	// Check for transmission errors
	// Expected first byte = 0xAA
	// Expected second Byte = b[7-4] is complement to b[3-0]
	if (answer[0] != 0xAA || (answer[1] & 0xF0) != (((~answer[1]) & 0x0F) << 4))
		throw OneWireWriteErrorException(deviceFileName);
}

void C1WireByKernel::ThreadWriteRawData8ChannelAddressableSwitch(const std::string& deviceFileName, int unit, bool value) const
{
	// First read actual state, to only change the correct unit
	// Caution : 0 means 'transistor active', we have to invert
	unsigned char otherPioState = ~ThreadReadRawData8ChannelAddressableSwitch(deviceFileName);

	unsigned char CmdWritePioRegisters[] = { 0x5A,0x00,0x00 };
	// 0 to activate the transistor
	unsigned char newPioState = value ? (otherPioState | (0x01 << unit)) : (otherPioState & ~(0x01 << unit));
	CmdWritePioRegisters[1] = ~newPioState;
	CmdWritePioRegisters[2] = ~CmdWritePioRegisters[1];

	unsigned char answer[2];
	sendAndReceiveByRwFile(deviceFileName, CmdWritePioRegisters, sizeof(CmdWritePioRegisters), answer, sizeof(answer));

	// Check for transmission errors
	// Expected first byte = 0xAA
	// Expected second Byte = inverted new pio state
	if (answer[0] != 0xAA || answer[1] != (unsigned char)(~newPioState))
		throw OneWireWriteErrorException(deviceFileName);
}

inline void std_to_upper(const std::string& str, std::string& converted)
{
	converted = "";
	for (char c : str)
		converted += (char)toupper(c);
}

void C1WireByKernel::GetDevice(const std::string& deviceName, /*out*/_t1WireDevice& device) const
{
	// 1W-Kernel device name format : ff-iiiiiiiiiiii, with :
	// - ff : family (1 byte)
	// - iiiiiiiiiiii : id (6 bytes)

	// Retrieve family from the first 2 chars
	device.family = ToFamily(deviceName.substr(0, 2));

	// Device Id (6 chars after '.')
	std_to_upper(deviceName.substr(3, 3 + 6 * 2), device.devid);

	// Filename (full path)
	device.filename = Wire1_Base_Dir;
	device.filename += "/" + deviceName;
}

