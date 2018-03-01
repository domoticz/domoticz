#pragma once

#include <iosfwd>
#include "ASyncTCP.h"
#include "DomoticzHardware.h"


#define MAX_EC3K_METERS 64
#define EC3K_UPDATE_INTERVAL 60

/*
typedef struct {
  int id;
  time_t last_update;
} Ec3kMeters_t;
*/

class Ec3kLimiter
{
public:
  Ec3kLimiter();
  ~Ec3kLimiter();
  bool update(int id);

private:
  int no_meters;
  struct {
	int id;
	time_t last_update;
	} meters[MAX_EC3K_METERS];
};

class Ec3kMeterTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	Ec3kMeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~Ec3kMeterTCP(void);
	bool isConnected(){ return mIsConnected; };
	bool WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	Ec3kLimiter *m_limiter;
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	void ParseData(const unsigned char *pData, int Len);
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

