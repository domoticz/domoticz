#pragma once
class MainWorker;

class CDataPush
{
public:
	void SetMainWorker(MainWorker *pMainWorker);
	void DoWork(const unsigned long long DeviceRowIdxIn);

private:
	unsigned long long DeviceRowIdx;
	MainWorker *m_pMain;	
	void DoFibaroPush();
};
