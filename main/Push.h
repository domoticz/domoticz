#pragma once

#include <boost/signals2.hpp>

class CPush
{
public:
	CPush();

	std::vector<std::string> DropdownOptions(const unsigned long long DeviceRowIdxIn);
	std::string DropdownOptionsValue(const unsigned long long DeviceRowIdxIn, const int pos);

protected:
	bool m_bLinkActive;
	unsigned long long m_DeviceRowIdx;
	boost::signals2::connection m_sConnection;

	const char *RFX_Type_SubType_DropdownOptions(const unsigned char dType, const unsigned char sType);
	std::string ProcessSendValue(const std::string &rawsendValue, const int delpos, const int nValue, const int includeUnit, const int metertype);
	std::string getUnit(const int delpos, const int metertypein);

	static void replaceAll(std::string& context, const std::string& from, const std::string& to);
};

