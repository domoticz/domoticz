#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

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
	std::vector<Action> m_vecActions;
	std::string toString()
	{
		std::string ret = "    Function: ";
		ret.append(m_strName);
		ret.append("\n      Commands:");
		for (auto &&action : m_vecActions)
		{
			ret.append("\n\t");
			ret.append(action.toString());
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
	std::vector<Function> m_vecFunctions;

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
		for (auto &&function : m_vecFunctions)
		{
			ret.append(function.toString());
		}
		return ret;
	}
};

class CHarmonyHub : public CDomoticzHardwareBase
{
	enum _eConnectionStatus
	{
		DISCONNECTED = 0,
		CONNECTED,
		INITIALIZED,
		AUTHENTICATED,
		BOUND
	};

      public:
	CHarmonyHub(int ID, const std::string &IPAddress, unsigned int port);
	~CHarmonyHub() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	// Init and cleanup
	void Init();
	void Logout();

	// Helper functions for changing switch status in Domoticz
	void CheckSetActivity(const std::string &activityID, bool on);
	void UpdateSwitch(unsigned char idx, const char *szIdx, bool bOn, const std::string &defaultname);

	// Raw socket functions
	bool ConnectToHarmony(const std::string &szHarmonyIPAddress, int HarmonyPortNumber, csocket *connection);
	bool SetupCommunicationSocket();
	void ResetCommunicationSocket();
	int WriteToSocket(std::string *szReq);
	std::string ReadFromSocket(csocket *connection);
	std::string ReadFromSocket(csocket *connection, float waitTime);

	// XMPP Stream initialization
	int StartStream(csocket *connection);
	int SendAuth(csocket *connection, const std::string &szUserName, const std::string &szPassword);
	int SendPairRequest(csocket *connection);
	int CloseStream(csocket *connection);

	// XMPP commands
	int SendPing();
	int SubmitCommand(const std::string &szCommand);
	int SubmitCommand(const std::string &szCommand, const std::string &szActivityId);

	// XMPP reading
	bool CheckForHarmonyData();
	void ParseHarmonyTransmission(std::string *szHarmonyData);
	void ProcessHarmonyConnect(std::string *szHarmonyData);
	void ProcessHarmonyMessage(std::string *szMessageBlock);
	void ProcessQueryResponse(std::string *szQueryResponse);

	// Helper function for XMPP reading
	bool IsTransmissionComplete(std::string *szHarmonyData);

      private:
	// hardware parameters
	std::string m_szHarmonyAddress;
	unsigned short m_usHarmonyPort;

	std::shared_ptr<std::thread> m_thread;
	std::mutex m_mutex;

	csocket *m_connection;
	_eConnectionStatus m_connectionstatus;

	bool m_bNeedMoreData;
	bool m_bWantAnswer;
	bool m_bNeedEcho;
	bool m_bReceivedMessage;
	bool m_bLoginNow;
	bool m_bIsChangingActivity;
	bool m_bShowConnectError;

	std::string m_szHubSwVersion;
	std::string m_szHarmonyData;
	std::string m_szAuthorizationToken;
	std::string m_szCurActivityID;
	std::map<std::string, std::string> m_mapActivities;
};
