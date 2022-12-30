#pragma once

#include "ThermostatHardware.h"

//duratoiopn of integration window in min
#define INTEGRAL_DURATION 10

// hors gel temperature celcius
#define TEMPERATURE_HG   8
// of temperature celcius
#define TEMPERATURE_OFF  0

enum VirtualThermostatMode {
	Eco=0,
	Confor,
  FrostProtection,
  Off,
  EndMode
};

#define AVALAIBLE_MODE "Eco,Conf,Frost,Off"

class VirtualThermostat : public CThermostatHardware
{
private:
class CircularBuffer {
public:
	double * Value;
	int Size; //size of buffer'
	int index; //current record index
	double Sum;

	CircularBuffer(int pSize);
	~CircularBuffer();
	int GetNext();
	//store value and return last
	double Put(double val);
	//get last value
	double GetLast();
	void Clear();
	double GetSum();
};

//
class LastValue {
	typedef std::map<int, double > T_Map_Double_Values;

	T_Map_Double_Values LastValues;
	double Delta;
public:

	LastValue(float delta = 0);
	double Get(int index);
	void   Put(int index, double value);
	bool AsChanged(int index, double value);
	bool AsChanged(int index, double value, double delta);

};

public:
	VirtualThermostat(const int ID);
	~VirtualThermostat();
	bool WriteToHardware(const char *pdata, const unsigned char length);

	void	ScheduleThermostat(int Minute);
	int		getPrevThermostatProg ( const char * devID , char * CurrentTime , std::string &Time );
	int		getNextThermostatProg ( const char * devID , char * CurrentTime , std::string &Time );
public:
    int ThermostatGetEcoConfort(const char* devID, int CurrentTargetTemp);
    void	ThermostatToggleEcoConfort (const char * devID , char * setTemp , char * Duration);
	int	ComputeThermostatOutput ( int Min , int PowerPercent );
    int ComputeThermostatPower(int index, double RoomTemp, double TargetTemp, double CoefProportional, double CoefIntegral);
	void getCommand(std::string& Cmd, std::string& OutCmd, int& level);
	int		GetConfortTempFromTimers  (const char * devID );
	int		GetEcoTempFromTimers  (const char * devID );
	std::string GetMode ( float curTemp , float EcoTemp, float ConfTemp );

	float GetEcoTemp (const char * devID );
	float GetConfortTemp (const char * devID );
    float getTemperatureFromSValue(const char * sValue);
    double ConvertTemperatureUnit(double tempcelcius);
    bool GetLastValue(  const char* DeviceID, int &nValue, std::string &sValue, struct tm &LastUpdateTime);

	typedef std::map<int,  CircularBuffer* > T_Map_CircularBuffer;

	T_Map_CircularBuffer DeltaTemps;

	//thermostat function

	//return the thermostat mode 
	virtual std::string GetCurrentMode(std::string &devIdx);
	//return the thermostat room temperature 
	virtual std::string GetRoomTemperature(std::string &devIdx);
    float GetSetPointTemperature(std::string& devIdx);
	std::string GetSetPoint(std::string &devIdx)  ;

	//set the thermostat mode 
	virtual bool SetThermostatState(const std::string &deviceIdx, const int newState);
	//convert interger state to string state : 0--> OFF 1-->ECO

  bool StartHardware();
  bool StopHardware();
  void Do_Work();

  std::shared_ptr<std::thread> m_thread;
  volatile bool m_stoprequested;
  int m_ScheduleLastMinute;

};

std::string VirtualThermostatGetOption (const std::string optionName , const std::string &options ,  const bool decode=true );

extern VirtualThermostat * m_VirtualThermostat;
