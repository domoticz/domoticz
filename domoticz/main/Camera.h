#pragma once

#include <string>
#include <vector>

class MainWorker;

struct cameraDevice
{
	unsigned long long ID;
	std::string Name;
    std::string Address;
	std::string Username;
	std::string Password;
	int Port;
	std::string VideoURL;
	std::string ImageURL;
};

class CCamScheduler
{
public:
	CCamScheduler(void);
	~CCamScheduler(void);
/*
	void StartCameraGrabber(MainWorker *pMainWorker);
	void StopCameraGrabber();
*/
    void ReloadCameras();
	std::vector<cameraDevice> GetCameraDevices();

	bool TakeSnapshot(const unsigned long long CamID, std::vector<unsigned char> &camimage);
	bool TakeSnapshot(const std::string CamID, std::vector<unsigned char> &camimage);
	cameraDevice* GetCamera(const unsigned long long CamID);
	cameraDevice* GetCamera(const std::string CamID);

	bool EmailCameraSnapshot(const std::string CamIdx, const std::string subject);

private:
	MainWorker *m_pMain;
	boost::mutex m_mutex;
	unsigned char m_seconds_counter;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	std::vector<cameraDevice> m_cameradevices;
/*
	//our thread
	void Do_Work();
	void CheckCameras();
*/
	std::string GetCameraURL(cameraDevice *pCamera);
};

