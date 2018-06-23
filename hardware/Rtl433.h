#pragma once

#include "DomoticzHardware.h"

class CRtl433 : public CDomoticzHardwareBase
{
public:
	explicit CRtl433(const int ID, const std::string &cmdline);
	virtual ~CRtl433();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	static std::vector<std::string> ParseCSVLine(const char *input);
private:
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	boost::mutex m_pipe_mutex;
	FILE *m_hPipe;
	std::string m_cmdline;
};
