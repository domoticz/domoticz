#pragma once

#include "DomoticzHardware.h"

class CRtl433 : public CDomoticzHardwareBase
{
public:
	explicit CRtl433(const int ID, const std::string &cmdline);
	virtual ~CRtl433();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	static std::vector<std::string> ParseCSVLine(const char *input);
	bool ParseLine(const std::vector<std::string> &headers, const char *line);
	bool FindField(const std::map<std::string, std::string> &data, const std::string &field);
private:
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_pipe_mutex;
	FILE *m_hPipe;
	std::string m_cmdline;
	std::string m_sLastLine;
	time_t m_time_last_received;
};
