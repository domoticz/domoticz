#include "stdafx.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"

#include "VirtualThermostat.h"

#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../notifications/NotificationHelper.h"
#include <stdarg.h>     

std::string AVALAIBLE_MODE = "Eco,Conf,Frost,Off";

void VirtualThermostat::CircularBuffer::Clear()
{
	for (unsigned int i = 0; i < m_Size; i++)m_Value[i] = 0;
	m_index = 0;
}
VirtualThermostat::CircularBuffer::CircularBuffer(unsigned int pSize)
{
	m_Value = new double[pSize];
	m_Size = pSize;
	Clear();
}
VirtualThermostat::CircularBuffer::~CircularBuffer()
{
	delete[] m_Value;
}
int VirtualThermostat::CircularBuffer::GetNext()
{
	if (m_index >= (m_Size - 1))
		return 0;
	else
		return m_index + 1;
}
double VirtualThermostat::CircularBuffer::Put(double val)
{
	double lastv = m_Value[m_index];
	m_Value[m_index] = val;
	m_index = GetNext();
	return lastv;
}

double VirtualThermostat::CircularBuffer::GetSum()
{
	double  Sum = 0;
	for (unsigned int i = 0; i < m_Size; i++)
	{
		Sum += m_Value[i];
	}
	return Sum;
}

//option management
void getOption(TOptionMap& Option, const char* optionName, int& value)
{
	try
	{
		value = std::stoi(Option[optionName]);
	}
	catch (const std::exception&)
	{
		value=0 ;
	}
}
void getOption(TOptionMap& Option, const char* optionName, long& value)
{
	try
	{
	value = std::stol(Option[optionName]);
	}
	catch (const std::exception&)
	{
		value=0 ;
	}
}
void getOption(TOptionMap& Option, const char* optionName, double& value)
{
	try
	{
	value = std::stof(Option[optionName]);
	}
	catch (const std::exception&)
	{
		value=0 ;
	}
}
void getOption(TOptionMap& Option, const char* optionName, std::string& value)
{
	value = (Option[optionName]);
}

void setOption(TOptionMap& Option, const char* optionName, int  value)
{
	Option[optionName] = std::to_string(value);
}
void setOption(TOptionMap& Option, const char* optionName, double  value)
{
	Option[optionName] = std_format("%4.1f", value);;
}
void setOption(TOptionMap& Option, const char* optionName, const char* value)
{
	Option[optionName] = value;
}
void setOption(TOptionMap& Option, const char* optionName, std::string& value)
{
	Option[optionName] = value;
}


VirtualThermostat* m_VirtualThermostat;

VirtualThermostat::VirtualThermostat(const int ID)
{
	m_HwdID = ID;
	m_VirtualThermostat = this;
	//thermostat mode string
	SetAvailableMode(AVALAIBLE_MODE);

}

VirtualThermostat::~VirtualThermostat()
{
	m_bIsStarted = false;
}
bool VirtualThermostat::StartHardware()
{
	RequestStart();

	m_stoprequested = false;

	//Start worker thread
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&VirtualThermostat::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	sOnConnected(this);
	m_bIsStarted = true;
	return (m_thread != NULL);
}
bool VirtualThermostat::StopHardware()
{
	m_stoprequested = true;
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}
void VirtualThermostat::Do_Work()
{
	int sec_counter = 0;
	Log(LOG_STATUS, "VirtualThermostat: Worker started...");
	while (IsStopRequested(1000) == false)
	{
		if (m_stoprequested)
			break;
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);
#ifndef ACCELERATED_TEST 
		if (ltime.tm_min != m_ScheduleLastMinute)
		{
			ScheduleThermostat(ltime.tm_min);
			m_ScheduleLastMinute = ltime.tm_min;
		}
#else
#pragma message("warning ACCELERATED_TEST")

		//test : call every 2 secconds
		if (ltime.tm_sec % 2 == 0)
		{
			ScheduleThermostat(ltime.tm_sec / 2);
			m_ScheduleLastMinute = ltime.tm_sec;
		}
#endif
	}
	Log(LOG_STATUS, "VirtualThermostat: Worker stopped...");
}
bool VirtualThermostat::WriteToHardware(const char* pdata, const unsigned char length)
{
	return true;
}

//time for the power time modulation in minute
constexpr int MODULATION_DURATION = 10;

//  number of step in percent %
constexpr int MODULATION_STEP = 10;

//compute the thermostat output switch value 
// the output is modulated in a time period of 10 minute
// each minute , the output is activated dependeing time and percent.
//input : Min : the minute counter 0..59
//input : PowerPercent : the power modulation  0..100%
//output 1 : activated

int VirtualThermostat::ComputeThermostatOutput(int Min, int PowerPercent)
{
	if (PowerPercent > 100) PowerPercent = 100;
	PowerPercent = (PowerPercent) / MODULATION_STEP;
	Min = Min % MODULATION_DURATION;
	if (Min >= PowerPercent)
		return 0;
	else
		return 1;
}

//return the power modulation in function of Room , Exterior and Target Temperature,
int VirtualThermostat::ComputeThermostatPower(int index, double RoomTemp, double TargetTemp, double CoefProportional, double CoefIntegral)
{
	int PowerModulation = 0;
	double DeltaTemp = TargetTemp - RoomTemp;
	CircularBuffer* Delta = m_DeltaTemps[index];
	if (Delta == 0)
	{
		Delta = new CircularBuffer(INTEGRAL_DURATION);
		m_DeltaTemps[index] = Delta;
	}
	//put last value
	Delta->Put(DeltaTemp);
	//need heating
	if (DeltaTemp > 0)
	{
		//PID regulation
		// coef integral is the sum of delta temperature since last 10 minutes.
		PowerModulation = int(DeltaTemp * CoefProportional + Delta->GetSum() * CoefIntegral / INTEGRAL_DURATION);
	}
	else
		Delta->Clear(); //clear integral part
	if (PowerModulation > 100) PowerModulation = 100;
	if (PowerModulation < 0) PowerModulation = 0;
	return PowerModulation;
}

//return the output command & level from cmd value
//cmd ='On;Off;Set Level XX "
void VirtualThermostat::getCommand(std::string& Cmd, std::string& OutCmd, int& level)
{
	OutCmd = Cmd; level = 0;

	if (Cmd == "Off") {
		level = 0;
	}
	else if (Cmd == "On") {
		level = 100;
	}
	else {
		std::vector<std::string> results;
		StringSplit(Cmd, " ", results);
		if (results.size() == 3) {
			if ((results[0] == "Set") && (results[1] == "Level")) {
				OutCmd = "Set Level";
				try {
					level = std::stoi(results[2]);
				}
				catch (...) {
					Log(LOG_ERROR, "Invalid Switch command : %s ", Cmd.c_str());
				}
			}
			else
				Log(LOG_ERROR, "Invalid Switch command : %s ", Cmd.c_str());
		}
		else
			Log(LOG_ERROR, "Invalid Switch command : %s ", Cmd.c_str());
	}
}

//get level from imput cmd : selectof value 
int translateCmd(std::string& Cmd, TOptionMap& options)
{
	int Level = -1;
	Level = GetSelectorSwitchLevel(options, Cmd);
	if (Level >= 0) {
		Cmd = "Set Level " + std::to_string(Level);
	}
	return Level;
}

void VirtualThermostat::ScheduleThermostat(int Minute)
{

	int SwitchValue = 0;
	int nValue, lastSwitchValue;
	std::string RoomTemperatureName;
	double ThermostatSetPoint = 0;
	const char* ThermostatName;
	struct tm LastUpdateTime;
	std::string sValue, Options;
	float RoomTemperature = 0;
	int ThermostatId;
	int PowerModulation = 0;
	int lastPowerModulation;
	double lastTemp;

	long  SwitchIdx;
	std::string SwitchIdxStr;

	double CoefProportional, CoefIntegral;
	std::string TemperatureId;
	std::string OnCmd;
	std::string OffCmd;
	long DeviceID;
	std::string sDeviceID;

	int switchtype;

	try
	{
		//select all the Virtuan device
		auto result = m_sql.safe_query("SELECT A.Name,A.ID,A.Type,A.SubType,A.nValue,A.sValue, A.Options, A.HardwareID , B.Type,A.DeviceID     FROM DeviceStatus as A LEFT OUTER JOIN Hardware as B ON (B.ID==A.HardwareID) where (B.type == %d )", HTYPE_VirtualThermostat);

		//for all the thermostat switch
		unsigned int nbDevices = result.size();

		for (unsigned int i = 0; i < nbDevices; i++)
		{
			double scrollDevice = double(MODULATION_DURATION) / nbDevices;
			TSqlRowQuery* row = &result[i];
			ThermostatName = (*row)[0].c_str();
			ThermostatId = std::stoi((*row)[1].c_str());

			//		SwitchType					= std::stoi((*row)[2].c_str() );
			//		SwitchSubType				= std::stoi((*row)[3].c_str() );
			lastSwitchValue = std::stoi((*row)[4].c_str());
			ThermostatSetPoint = std::stof((*row)[5].c_str());
			Options = ((*row)[6].c_str());
			DeviceID = std::stol((*row)[9], 0, 16);
			sDeviceID = (*row)[9];

			TOptionMap Option = m_sql.BuildDeviceOptions(Options);

			getOption(Option, "Power", lastPowerModulation);
			getOption(Option, "RoomTemp", lastTemp);
			getOption(Option, "TempIdx", TemperatureId);
			getOption(Option, "SwitchIdx", SwitchIdx);
			getOption(Option, "SwitchIdx", SwitchIdxStr);
			getOption(Option, "CoefProp", CoefProportional); //coef for propotianal command PID
			getOption(Option, "CoefInteg", CoefIntegral);        //coef for propotianal command PID
			getOption(Option, "OnCmd", OnCmd);      //switch command for power On the radiator
			getOption(Option, "OffCmd", OffCmd);     //switch command for power Off the radiator

			if (SwitchIdx <= 0) {
				Log(LOG_ERROR, "No Switch device associted to Thermostat  name =%s", ThermostatName);
			}
			else if (ThermostatSetPoint == 0)
			{
				if ((Minute % 10) == 0)
					Debug(DEBUG_NORM, "VTHER: Mn:%02d  Therm:%-10s(%2d) SetPoint:%4.1f POWER OFF LightId(%2ld):%d ", Minute, ThermostatName, ThermostatId, ThermostatSetPoint, SwitchIdx, SwitchValue);

			}
			//retrieve corresponding Temperature device name    
			//the temperture corresponding device is stored in LightSubDevice table
			else if (std::stoi(TemperatureId.c_str()) > 0)
			{
				//get current room temperature  
				if (GetLastValue(TemperatureId.c_str(), nValue, sValue, LastUpdateTime))
				{
					RoomTemperature = getTemperatureFromSValue(sValue.c_str());
					PowerModulation = ComputeThermostatPower(ThermostatId, RoomTemperature, ThermostatSetPoint, CoefProportional, CoefIntegral);

					//minute+i : shift  device in time in order to not have all powered together
					int minute = (int)(double(Minute) + double(i) * scrollDevice);

					SwitchValue = ComputeThermostatOutput(minute, PowerModulation);

					//force to update state in database else only send RF commande with out database update 
					//if the switch is a HomeEasy protocol with no RF acknoledge , send the RF command each minute with out database DEVICESTATUS table update  
					auto resSw = m_sql.safe_query("SELECT nValue,Type,SubType,SwitchType,LastLevel,Name FROM DeviceStatus WHERE (ID == %s )", SwitchIdxStr.c_str());

					if (!resSw.empty())
					{
						//SwitchSubType    = std::stoi(resSw[0][2].c_str());
						lastSwitchValue = std::stoi(resSw[0][0].c_str());
						switchtype = std::stoi(resSw[0][3].c_str());
						int LastLevel = std::stoi(resSw[0][4].c_str());
						std::string SwitchName = resSw[0][5];
						std::string OutCmd; int level;

						if (switchtype == STYPE_Selector)
						{
							//gey device option of switch
							TOptionMap options = m_sql.GetDeviceOptions(SwitchIdxStr);
							translateCmd(OnCmd, options);
							translateCmd(OffCmd, options);
						}
						if (SwitchValue == 1)
							getCommand(OnCmd, OutCmd, level);
						else
							getCommand(OffCmd, OutCmd, level);

						bool SwitchStateAsChanged = false;

						//check if command switch state as changed
						if ((level == 0) && (lastSwitchValue > 0))
							SwitchStateAsChanged = true;
						if ((level == 100) && (lastSwitchValue != 1))
							SwitchStateAsChanged = true;
						if ((lastSwitchValue == 2)) {
							if (LastLevel != level)
								SwitchStateAsChanged = true;
						}
						if ((lastSwitchValue == 0)) {
							if (level > 0)
								SwitchStateAsChanged = true;
						}

						if ((minute % 10) == 0 || (SwitchStateAsChanged))
						{
							m_mainworker.SwitchLight(SwitchIdx, OutCmd, level, _tColor(), false, 0, "VTHER" /*, !SwitchStateAsChanged */);
							sleep_milliseconds(100);
							//						Debug(DEBUG_NORM,"VTHER: Mn:%02d  Therm:%-10s(%2d) Room:%4.1f SetPoint:%4.1f Power:%3d%% SwitchName:%s(%2ld):%d Kp:%3.f Ki:%3.f Integr:%3.1f Cmd:%s Level:%d",Minute,ThermostatName, ThermostatId , RoomTemperature,ThermostatSetPoint,PowerModulation,SwitchName.c_str(),SwitchIdx,SwitchValue,CoefProportional,CoefIntegral, m_DeltaTemps[ThermostatId]->GetSum() /INTEGRAL_DURATION, OutCmd.c_str(),level);
							SwitchStateAsChanged = true;
						}
						if ((abs(lastPowerModulation - PowerModulation) > 10) || (abs(lastTemp - RoomTemperature) > 0.2) || (SwitchStateAsChanged))
						{
							setOption(Option, "Power", PowerModulation);
							setOption(Option, "RoomTemp", RoomTemperature);
							setOption(Option, "Switch", SwitchValue);     //switch command value
							m_sql.SetDeviceOptions(ThermostatId, Option);

							//force display refresh
							SendSetPointSensor((uint8_t)(DeviceID >> 16), (DeviceID >> 8) & 0xFF, (DeviceID) & 0xFF, (float)ThermostatSetPoint, "");
							Debug(DEBUG_NORM, "VTHER: Mn:%02d  Therm:%-10s(%2d) Room:%4.1f SetPoint:%4.1f Power:%3d%% SwitchName:%s(%2ld):%d Kp:%3.f Ki:%3.f Integr:%3.2f Cmd:%s Level:%d", Minute, ThermostatName, ThermostatId, RoomTemperature, ThermostatSetPoint, PowerModulation, SwitchName.c_str(), SwitchIdx, SwitchValue, CoefProportional, CoefIntegral, m_DeltaTemps[ThermostatId]->GetSum() / INTEGRAL_DURATION, OutCmd.c_str(), level);
						}
						else
						{
#ifdef ACCELERATED_TEST 
							Debug(DEBUG_NORM, "VTHER: Mn:%02d  Therm:%-10s(%2d) Room:%4.1f SetPoint:%4.1f Power:%3d%% SwitchName:%s(%2ld):%d Kp:%3.f Ki:%3.f Integr:%3.2f Cmd:%s Level:%d", Minute, ThermostatName, ThermostatId, RoomTemperature, ThermostatSetPoint, PowerModulation, SwitchName.c_str(), SwitchIdx, SwitchValue, CoefProportional, CoefIntegral, m_DeltaTemps[ThermostatId]->GetSum() / INTEGRAL_DURATION, OutCmd.c_str(), level);
#endif
						}
						//if ((Minute % 10 )==0)
						//{
						   // //compute delta room temperature
						   // float DeltaTemp = RoomTemperature - Map_LastRoomTemp[ThermostatId] ;
						   // Map_LastRoomTemp[ThermostatId] = RoomTemperature ;
						   // if ( DeltaTemp == RoomTemperature ) DeltaTemp=0;  //first call
						//		auto res=m_sql.safe_query("SELECT ID FROM DeviceStatus where Name=='his_%s'", ThermostatName ) ;
						//		if (res.size() )m_sql.safe_query( "INSERT INTO Temperature (DeviceRowID, Temperature, Humidity ) VALUES (%s , %4.1f, %d )",res[0][0].c_str(),ThermostatSetPoint,PowerModulation	);
						//}
					}
					else
						Log(LOG_ERROR, "No switch device associted to Thermostat name =%s", ThermostatName);
				}
			}
			else
				Log(LOG_ERROR, "No temperature device associted to Thermostat name =%s", ThermostatName);
		}//nd for
	}
	catch (...)
	{
		Log(LOG_ERROR, "Exception in ScheduleThermostat");
	}
}

//return the option value from devive id 
std::string GetOptionFromDeviceId(const std::string& devIdx, const char* option)
{
	TOptionMap Option = m_sql.GetDeviceOptions(devIdx);
	return (Option[option]);
}

//return previous  thermostat target temperature before Time
int VirtualThermostat::getPrevThermostatProg(const char* devID, char* CurrentTime, std::string& Time)
{
	int TargetTemp = 0;
	auto result = m_sql.safe_query("SELECT Time,Temperature FROM SetpointTimers where (DeviceRowID==%s) and ( Time < '%s' ) order by time desc limit 1", devID, CurrentTime);
	if (!result.empty()) {
		Time = result[0][0];
		TargetTemp = std::stoi(result[0][1].c_str());
	}
	return TargetTemp;
}

//return next  thermostat target temperature after Time
int VirtualThermostat::getNextThermostatProg(const char* devID, char* CurrentTime, std::string& Time)
{
	int TargetTemp = 0;
	auto result = m_sql.safe_query("SELECT Time,Temperature FROM SetpointTimers where (DeviceRowID==%s) and ( Time > '%s' ) order by time asc limit 1", devID, CurrentTime);
	if (!result.empty()) {
		Time = result[0][0];
		TargetTemp = std::stoi(result[0][1].c_str());
	}
	return TargetTemp;
}

float VirtualThermostat::GetEcoTemp(const char* devID)
{
	std::string sid = devID;
	std::string temp = GetOptionFromDeviceId(sid, "EcoTemp");

	return ((float)std::stof(temp.c_str()));
}

int VirtualThermostat::GetEcoTempFromTimers(const char* devID)
{
	int MinTemp;
	//get min temperature  from SetpointTimers
	auto result = m_sql.safe_query("SELECT MIN(Temperature) FROM SetpointTimers where DeviceRowID==%s", devID);
	if (!result.empty()) MinTemp = std::stoi(result[0][0].c_str()); else MinTemp = 16;
	return MinTemp;
}

float VirtualThermostat::GetConfortTemp(const char* devID)
{
	std::string sid = devID;
	std::string temp = GetOptionFromDeviceId(sid, "ConforTemp");
	return (float)std::stof(temp.c_str());
}

int VirtualThermostat::GetConfortTempFromTimers(const char* devID)
{
	int MaxTemp;
	//get  max temperature  from SetpointTimers
	auto result = m_sql.safe_query("SELECT MAX(Temperature) FROM SetpointTimers where DeviceRowID==%s", devID);
	if (!result.empty()) MaxTemp = std::stoi(result[0][0].c_str()); else MaxTemp = 20;
	return MaxTemp;
}

//force : toggle the thermostat temperature state to ECO / CONFORT mode
//algo  if current target temperature = minimum values in Timers table content  then
//      next Target = Maximum values in Timers table content 
//algo  if current target temperature = Maximum values in Timers table content  then
//      next Target = minimum values in Timers table content 

int VirtualThermostat::ThermostatGetEcoConfort(const char* devID, int CurrentTargetTemp)
{

	int NextTargetTemp, MinTemp, MaxTemp;

	MaxTemp = (int)GetConfortTemp(devID);
	MinTemp = (int)GetEcoTemp(devID);

	NextTargetTemp = CurrentTargetTemp;
	int moy = (MaxTemp + MinTemp) / 2;
	if (CurrentTargetTemp <= moy)  NextTargetTemp = MaxTemp;
	else                         NextTargetTemp = MinTemp;

	return NextTargetTemp;

}

void VirtualThermostat::ThermostatToggleEcoConfort(const char* devID, char* setTemp, char* Duration)
{
	int CurrentTargetTemp = std::stoi(setTemp);;
	int targetTemp = ThermostatGetEcoConfort(devID, CurrentTargetTemp);
	//update the thermostat set point : Cmd Set temp
	std::string ID = devID;
	m_sql.UpdateDeviceValue("sValue", (int)targetTemp, (ID));
	Debug(DEBUG_NORM, "VTHER: Toggle Eco Confort Idx:%s Temp:%d Duration:%s", devID, targetTemp, Duration);
}

//return true if in confor mode
std::string VirtualThermostat::GetCurrentMode(const std::string& devIdx)
{

	float EcoTemp = GetEcoTemp(devIdx.c_str());
	float ConforTemp = GetConfortTemp(devIdx.c_str());
	float RoomSetPointTemp = GetSetPointTemperature(devIdx);

	return GetMode(RoomSetPointTemp, EcoTemp, ConforTemp);
};

std::string VirtualThermostat::GetMode(float curTemp, float EcoTemp, float ConfTemp)
{
	float  moy = (EcoTemp + ConfTemp) / 2;
	if (curTemp <= moy)  return ThermostatModeIntToString(Eco);
	else               return ThermostatModeIntToString(Confor);
}

bool VirtualThermostat::SetThermostatState(const std::string& idx, const unsigned int newState)
{
	VirtualThermostatMode mode = (VirtualThermostatMode)newState;
	if (mode >= EndMode)
		return false;
	if (mode == Confor)
		m_mainworker.SetSetPoint(idx, (float)m_VirtualThermostat->GetConfortTemp(idx.c_str()));
	else   if (mode == Eco)
		m_mainworker.SetSetPoint(idx, (float)m_VirtualThermostat->GetEcoTemp(idx.c_str()));
	else   if (mode == FrostProtection)
		m_mainworker.SetSetPoint(idx, (float)ConvertTemperatureUnit((float)TEMPERATURE_HG));
	else   if (mode == Off)
		m_mainworker.SetSetPoint(idx, (float)ConvertTemperatureUnit((float)TEMPERATURE_OFF));
	return true;
}

//return the thermostat room temperature 
std::string VirtualThermostat::GetRoomTemperature(const std::string& devIdx)
{
	std::string temp = GetOptionFromDeviceId(devIdx, "RoomTemp");
	return (temp);
};

std::string VirtualThermostat::GetSetPoint(const std::string& devIdx)
{
	return (m_sql.GetDeviceValue("svalue", devIdx.c_str()).c_str());
}

float VirtualThermostat::GetSetPointTemperature(const std::string& devIdx)
{
	return ((float)std::stof(GetSetPoint(devIdx).c_str()));
};

//option is not in base64
std::string VirtualThermostatGetOption(const std::string optionName, const std::string& options, const bool decode)
{
	//if option present
	TOptionMap Option = m_sql.BuildDeviceOptions(options, decode);
	return (Option[optionName]);
}
//return temperature value from Svalue : is code temperature;humidity;???
float VirtualThermostat::getTemperatureFromSValue(const char* sValue)
{
	std::vector<std::string> splitresults;
	StringSplit(sValue, ";", splitresults);
	if (splitresults.size() < 1)
		return 0;
	else
		return (float)std::stof(splitresults[0].c_str());
}
//convert from  celcius to farenheit if unit=farenheit
double  VirtualThermostat::ConvertTemperatureUnit(double tempcelcius)
{
	if (m_sql.m_tempunit == TEMPUNIT_F)
	{
		//Convert back to celcius
		tempcelcius = ConvertToFahrenheit(tempcelcius);
	}
	return tempcelcius;
}
bool VirtualThermostat::GetLastValue(const char* DeviceID, int& nValue, std::string& sValue, struct tm& LastUpdateTime)
{
	bool result = false;
	char szTmp[400];
	std::string sLastUpdate;
	//std::string sValue;
	//struct tm LastUpdateTime;
	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now, &tm1);

	sprintf(szTmp, "SELECT nValue,sValue,LastUpdate FROM DeviceStatus WHERE ( ID=%s ) ", DeviceID);
	auto sqlresult = m_sql.safe_query(szTmp);

	if (!sqlresult.empty())
	{
		nValue = (int)std::stoi(sqlresult[0][0].c_str());
		sValue = sqlresult[0][1];

		sLastUpdate = sqlresult[0][2];

		time_t lutime;
		ParseSQLdatetime(lutime, LastUpdateTime, sLastUpdate);

		result = true;
	}

	return result;
}

/// @brief           build option map 
///					 ex : BuildDeviceOptions ( 1,"optionName1", "optionValue1")
///					 ex : BuildDeviceOptions ( 2,"optionName1", "optionValue1","optionName2", "optionValue2")
/// @param NbOptions option couple number
/// @param  
/// @return 
TOptionMap  BuildDeviceOptions(int NbOptions, ...)
{
	std::map<std::string, std::string> optionsMap;

	char* optionName, * optionValue;

	va_list vl;
	va_start(vl, NbOptions);
	for (int i = 0; i < NbOptions; i++)
	{
		optionName = va_arg(vl, char*);
		optionValue = va_arg(vl, char*);
		optionsMap.insert(std::pair<std::string, std::string>(optionName, optionValue));

	}
	va_end(vl);

	return optionsMap;
}
