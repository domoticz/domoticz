#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>
#include "hardwaretypes.h"

#define BUFFER_SIZE								2*1024*1024

class csocket;

class Action
{
public:
    std::string m_strCommand;
    std::string m_strName;
    std::string m_strLabel;
    std::string toString()
    {
        return m_strCommand;
    }
};


class Function
{
public:
    std::string m_strName;
    std::vector< Action > m_vecActions;
    std::string toString()
    {
        std::string ret = "    Function: ";
        ret.append(m_strName);
        ret.append("\n      Commands:");
        std::vector<Action>::iterator it = m_vecActions.begin();
        std::vector<Action>::iterator ite = m_vecActions.end();
        for(; it != ite; ++it)
        {
            ret.append("\n\t");
            ret.append(it->toString());
        }
        ret.append("\n");
        return ret;
    }
};

class Device
{
public:
    std::string m_strID;
    std::string m_strLabel;
    std::string m_strManufacturer;
    std::string m_strModel;
    std::string m_strType;
    std::vector< Function > m_vecFunctions;

    std::string toString()
    {
        std::string ret = m_strType;
        ret.append(": ");
        ret.append(m_strLabel);
        ret.append(" (ID = ");
        ret.append(m_strID);
        ret.append(")\n");
        ret.append(m_strManufacturer);
        ret.append(" - ");
        ret.append(m_strModel);
        ret.append("\nFunctions: \n");
        std::vector<Function>::iterator it = m_vecFunctions.begin();
        std::vector<Function>::iterator ite = m_vecFunctions.end();
        for(; it != ite; ++it)
        {
            ret.append(it->toString());
        }
        return ret;
    }
};

class CHarmonyHub : public CDomoticzHardwareBase
{
public:
	CHarmonyHub(const int ID, const std::string &IPAddress, const unsigned int port);
	~CHarmonyHub(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool Login();
	void Logout();
	bool SetupCommandSocket();
	bool UpdateActivities();
	bool UpdateCurrentActivity();
	void CheckSetActivity(const std::string &activityID, const bool on);
	void UpdateSwitch(const unsigned char idx, const char * szIdx, const bool bOn, const std::string &defaultname);
	
	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	bool ConnectToHarmony(const std::string &strHarmonyIPAddress, const int harmonyPortNumber, csocket* harmonyCommunicationcsocket);
	bool StartCommunication(csocket* communicationcsocket, const std::string &strUserName, const std::string &strPassword);
	bool GetAuthorizationToken(csocket* authorizationcsocket);
	bool SubmitCommand(const std::string &strCommand, const std::string &strCommandParameterPrimary, const std::string &strCommandParameterSecondary);
	bool CheckIfChanging(const std::string& strData);
	bool SendPing();
	bool ParseAction(const std::string& strAction, std::vector<Action>& vecDeviceActions, const std::string& strDeviceID);
	//bool ParseFunction(const std::string& strFunction, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID);

	std::string m_harmonyAddress;
	unsigned short m_usIPPort;
	std::string m_szAuthorizationToken;
	std::string m_szCurActivityID;
	boost::mutex m_mutex;

	csocket * m_commandcsocket;
	volatile bool m_stoprequested;
	bool m_bDoLogin;
	bool m_bIsChangingActivity;
	std::string m_hubSwVersion;
	boost::shared_ptr<boost::thread> m_thread;
	char m_databuffer[BUFFER_SIZE];
	std::string m_szResultString;
};
