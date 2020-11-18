#pragma once

#include "DomoticzHardware.h"

class CRtl433 : public CDomoticzHardwareBase
{
      public:
	explicit CRtl433(int ID, const std::string &cmdline);
	~CRtl433() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool ParseJsonLine(const std::string &sLine);
	bool FindField(const std::map<std::string, std::string> &data, const std::string &field);
	bool ParseData(std::map<std::string, std::string> &data);

      private:
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_pipe_mutex;
	std::string m_cmdline;
	std::string m_sLastLine;
};
