#pragma once

#include <string>
#include <vector>

class MainWorker;

struct cameraDevice
{
	unsigned long long DevID;
	std::string Name;
    std::string Address;
	std::string Username;
	std::string Password;
	int Port;
};

class CCamScheduler
{
public:
	CCamScheduler(void);
	~CCamScheduler(void);

	void StartCameraGrabber(MainWorker *pMainWorker);
	void StopCameraGrabber();
    void ReloadCameras();
    void CheckCameras();
	std::vector<cameraDevice> GetCameraDevices();

private:
	MainWorker *m_pMain;
	boost::mutex m_mutex;
	unsigned char m_seconds_counter;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	std::vector<cameraDevice> m_cameradevices;

	//our thread
	void Do_Work();


};

