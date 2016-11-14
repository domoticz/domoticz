#pragma once

#include "DomoticzHardware.h"
#include "ASyncTCP.h"
#include <deque>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class Yeelight : public CDomoticzHardwareBase
{
public:
	Yeelight(const int ID);
	~Yeelight(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void InsertUpdateSwitch(const std::string &SketchName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue);
	void UpdateSwitch(const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue, const std::string &yeelightRgb);

public:
	//signals
	boost::signals2::signal<void()> sDisconnected;

private:
	bool StartHardware();
	bool StopHardware();
protected:
	bool m_bDoRestart;
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

public:
	class YeelightUDP
	{
	public:
		YeelightUDP(boost::asio::io_service & io_service, int HardwareID);
		boost::asio::ip::udp::socket socket_;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		void start_send();

	private:
		void start_receive();
		bool HandleIncoming(const std::string &szData);

	protected:
		int m_HwdID;
	};

	class YeelightTCP : public ASyncTCP
	{
	public:
		YeelightTCP(const std::string DeviceID, int HardwareID, int YeeType);
		~YeelightTCP(void);
		bool isConnected() { return mIsConnected; };
		bool WriteInt(const std::string &sendString);
		bool WriteInt(const uint8_t *pData, const size_t length);
		bool Start();
		bool Stop();
		std::string m_szDeviceId;

	public:
		boost::signals2::signal<void()>	sDisconnected;

	protected:
		bool m_bDoRestart;
		void Do_Work();
		void OnConnect();
		void OnDisconnect();
		void OnData(const unsigned char *pData, size_t length);
		void OnError(const std::exception e);
		void OnError(const boost::system::error_code& error);
		boost::shared_ptr<boost::thread> m_thread;
		volatile bool m_stoprequested;
		std::string m_szIPAddress;
		int m_HwdID;
		unsigned short m_usIPPort;
		int m_Type;
	};

};