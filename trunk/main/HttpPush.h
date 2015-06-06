#pragma once

#include <boost/signals2.hpp>

class CHttpPush
{
public:
	CHttpPush();
	void Start();
	void Stop();

	void UpdateActive();
	std::vector<std::string> DropdownOptions(const unsigned long long DeviceRowIdxIn);
	std::string DropdownOptionsValue(const unsigned long long DeviceRowIdxIn, const int pos);

private:
	bool m_bHttpLinkActive;
	unsigned long long m_DeviceRowIdx;
	boost::signals2::connection m_sConnection;

	void OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void DoHttpPush();
	const char *RFX_Type_SubType_DropdownOptions(const unsigned char dType, const unsigned char sType);
	std::string getUnit(const int delpos, const int metertype);

	static void replaceAll(std::string& context, const std::string& from, const std::string& to);
};

