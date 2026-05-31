#pragma once

#define REMOTE_PROTOCOL_VERSION 3

#include "DomoticzHardware.h"
#include "RFXBase.h"
#if defined WIN32
#include "ws2tcpip.h"
#endif
#include "ASyncTCP.h"

class DomoticzTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	DomoticzTCP(int ID, const std::string& IPAddress, unsigned short usIPPort, const std::string& username, const std::string& password);
	~DomoticzTCP() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;
	bool SwitchLight(const uint64_t idx, const std::string& switchcmd, const int level, _tColor color, const bool ooc, const std::string& User);
	bool SetSetPoint(const std::string& idx, const float TempValue, const std::string& User = "");
	bool SetSetPointEvo(const std::string& idx, float TempValue, const std::string& newMode, const std::string& until, const std::string& User = "");
	bool SetThermostatState(const std::string& idx, int newState);
	bool SwitchEvoModal(const std::string& idx, const std::string& status, const std::string& action, const std::string& ooc, const std::string& until);
	bool SetTextDevice(const std::string& idx, const std::string& text);

#ifdef WITH_OPENZWAVE
	bool SetZWaveThermostatMode(const std::string& idx, int tMode);
	bool SetZWaveThermostatFanMode(const std::string& idx, int fMode);
#endif

private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool WriteToHardware(const std::string& szData);
	void WriteFramed(const std::string& szData);
	bool SendEncrypted(const std::string& szPlaintext);

	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_username;
	std::string m_password;
	std::mutex m_readMutex;
	std::string m_recvBuffer;
	time_t m_tAuthSent = 0;
	bool m_bDataReceived = false;
	std::shared_ptr<std::thread> m_thread;
protected:
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const uint8_t* pData, size_t length) override;
	void OnError(const boost::system::error_code& error) override;
};
