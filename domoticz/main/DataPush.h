#pragma once
class MainWorker;

class CDataPush
{
public:
	void SetMainWorker(MainWorker *pMainWorker);
	void DoWork(const unsigned long long DeviceRowIdxIn);
	std::vector<std::string> DropdownOptions(const unsigned long long DeviceRowIdxIn);
	std::string DropdownOptionsValue(const unsigned long long DeviceRowIdxIn,int pos);

private:
	unsigned long long DeviceRowIdx;
	MainWorker *m_pMain;	
	void DoFibaroPush();
	const char *RFX_Type_SubType_DropdownOptions(const unsigned char dType, const unsigned char sType);
	std::string ProcessSValue(std::string rawsendValue, int delpos, int includeUnit, int metertype);
};
