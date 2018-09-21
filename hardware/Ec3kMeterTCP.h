#pragma once

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
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void ParseData(const unsigned char *pData, int Len);

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
private:
	int m_retrycntr;
	Ec3kLimiter *m_limiter;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	std::shared_ptr<std::thread> m_thread;
};

