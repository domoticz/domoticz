#pragma once

#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"
#include <boost/thread/thread_time.hpp>

#define CONFIG_NR_OF_PARAMETER_TYPES 13
#define CONFIG_NR_OF_PARAMETER_BOOL_TYPES 4
#define CONFIG_NR_OF_PARAMETER_PIN_TYPES 4
#define CONFIG_NR_OF_PARAMETER_COUNT_TYPES 3

#define LEVEL 0
#define INV_LEVEL 1
#define TOGGLE_RISING 2
#define TOGGLE_FALLING 3

#define COUNT_TYPE_GENERIC 0
#define COUNT_TYPE_RFXMETER 1
#define COUNT_TYPE_ENERGY 2

class CIOCount
{
	friend class CIOPort;

      public:
	CIOCount();
	~CIOCount() = default;

	int Update(unsigned long Counts);
	unsigned long GetTotal() const
	{
		return Total;
	};
	unsigned long GetLastTotal() const
	{
		return LastTotal;
	};
	unsigned long GetRateLimit() const
	{
		return Minimum_Pulse_Period_ms;
	};
	unsigned long GetDivider() const
	{
		return Divider;
	};
	void SetTotal(unsigned long NewTotalValue)
	{
		Total = NewTotalValue;
	};
	void SetLastTotal(unsigned long NewTotalValue)
	{
		LastTotal = NewTotalValue;
	};
	void SetRateLimit(unsigned long NewRateLimit)
	{
		Minimum_Pulse_Period_ms = NewRateLimit;
	};
	void ResetTotal()
	{
		Total = 0;
		LastTotal = 0;
	};
	bool ProcessUpdateInterval(unsigned long PassedTime_ms);
	void SetUpdateInterval(unsigned long NewValue_ms);
	void SetUpdateIntervalPerc(unsigned long NewValue)
	{
		UpdateIntervalPerc = NewValue;
	};
	void SetDivider(unsigned long NewValue)
	{
		Divider = NewValue;
	};
	unsigned long GetUpdateInterval()
	{
		return UpdateInterval_ms;
	};
	unsigned long GetUpdateIntervalPerc()
	{
		return UpdateIntervalPerc;
	};
	bool Enabled;
	bool InitialStateSent;
	int Type;

      private:
	unsigned long Total;
	unsigned long LastTotal;
	unsigned long UpdateInterval_ms;
	unsigned long UpdateDownCount_ms;
	unsigned long UpdateIntervalPerc;
	unsigned long Minimum_Pulse_Period_ms;
	unsigned long Divider;
	boost::posix_time::ptime Last_Pulse;
	boost::posix_time::ptime Cur_Pulse;
	boost::posix_time::ptime Last_Callback;
	boost::posix_time::time_duration Last_Interval;
	boost::posix_time::time_duration Cur_Interval;
};

class CIOPinState
{
	friend class CIOPort;

      public:
	CIOPinState();
	~CIOPinState() = default;

	int Update(bool New);
	int UpdateInterrupt(bool IntFlag, bool PinState);
	int GetInitialState();
	bool GetCurrent() const
	{
		return Current;
	}
	bool Enabled;
	int Id;
	int Type;
	CIOCount Count;
	bool InitialStateSent;
	unsigned char Direction;

      private:
	tRBUF IOPinCounterPacket;
	bool Last;
	bool Current;
	bool Toggle;
};

class CIOPort
{
	friend class CPiFace;

      public:
	CIOPort();
	~CIOPort() = default;
	int Update(unsigned char New);
	int UpdateInterrupt(unsigned char IntFlag, unsigned char PinState);
	unsigned char GetCurrent() const
	{
		return Current;
	}
	int GetDevId() const
	{
		return devId;
	}
	bool IsDevicePresent() const
	{
		return Present;
	}
	void Init(bool Available, int hwdId, int devId, unsigned char housecode, unsigned char initial_state);
	void SetID(int devID)
	{
		devId = devID;
	}
	void ConfigureCounter(unsigned char Pin, bool Enable);
	void *Callback_pntr;
	CIOPinState Pin[8];
	unsigned int PortType;

      private:
	tRBUF IOPinStatusPacket;
	unsigned char Last;
	unsigned char Current;
	int devId;
	bool Present;
};

class CPiFace : public CDomoticzHardwareBase
{
	friend class CIOPort;

      public:
	explicit CPiFace(int ID);
	~CPiFace() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;

	void Do_Work();
	void Do_Work_Queue();

	void CallBackSendEvent(const unsigned char *pEventPacket, unsigned int PacketLength);
	void CallBackSetPinInterruptMode(unsigned char devId, unsigned char pinID, bool Interrupt_Enable);

	int Init_SPI_Device(int Init);
	void Init_Hardware(unsigned char devId);
	int Read_Write_SPI_Byte(unsigned char *data, int len);
	int Read_MCP23S17_Register(unsigned char devId, unsigned char reg);
	int Write_MCP23S17_Register(unsigned char devId, unsigned char reg, unsigned char data);
	void Sample_and_Process_Inputs(unsigned char devId);
	void Sample_and_Process_Outputs(unsigned char devId);
	void Sample_and_Process_Input_Interrupts(unsigned char devId);
	void GetAndSetInitialDeviceState(int devId);

	int Detect_PiFace_Hardware();

	std::string &preprocess(std::string &s);
	std::string &trim(std::string &s);
	std::string &ltrim(std::string &s);
	std::string &rtrim(std::string &s);
	int LocateValueInParameterArray(const std::string &Parametername, const std::string *ParameterArray, int Items);
	int GetParameterString(const std::string &TargetString, const char *SearchStr, int StartPos, std::string &Parameter);
	int LoadConfig();
	void LoadDefaultConfig();
	void AutoCreate_piface_config();

      private:
	std::shared_ptr<std::thread> m_thread;
	std::shared_ptr<std::thread> m_queue_thread;
	StoppableTask m_TaskQueue;

	int m_InputSample_waitcntr;
	int m_CounterEdgeSample_waitcntr;

	int m_fd;

	CIOPort m_Inputs[4];
	CIOPort m_Outputs[4];

	bool m_DetectedHardware[4];

	// config file functions
	static const std::string ParameterNames[CONFIG_NR_OF_PARAMETER_TYPES];
	static const std::string ParameterBooleanValueNames[CONFIG_NR_OF_PARAMETER_BOOL_TYPES];
	static const std::string ParameterPinTypeValueNames[CONFIG_NR_OF_PARAMETER_PIN_TYPES];
	static const std::string ParameterCountTypeValueNames[CONFIG_NR_OF_PARAMETER_COUNT_TYPES];

	std::mutex m_queue_mutex;
	std::vector<std::string> m_send_queue;
};
