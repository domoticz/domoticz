#pragma once

#include <boost/signals2.hpp>

class CBasePush
{
public:
	CBasePush();

	static std::vector<std::string> DropdownOptions(const uint64_t DeviceRowIdxIn);
	static std::string DropdownOptionsValue(const uint64_t DeviceRowIdxIn, const int pos);
protected:
	bool m_bLinkActive;
	uint64_t m_DeviceRowIdx;
	boost::signals2::connection m_sConnection;
	boost::signals2::connection m_sNotification;

	std::string ProcessSendValue(const std::string &rawsendValue, const int delpos, const int nValue, const int includeUnit, const int devType, const int devSubType, const int metertype);
	std::string getUnit(const int delpos, const int metertypein);

	static unsigned long get_tzoffset();
#ifdef WIN32
	static std::string get_lastUpdate(unsigned __int64);
#else
	static std::string get_lastUpdate(unsigned long long int);
#endif

	static void replaceAll(std::string& context, const std::string& from, const std::string& to);
};

