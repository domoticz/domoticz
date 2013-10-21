#pragma once

#include <string>
#include <vector>

class MainWorker;

struct cameraActiveDevice
{
	unsigned long long ID;
	unsigned long long DevSceneRowID;
	unsigned char DevSceneType;
};

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
	std::vector<cameraActiveDevice> mActiveDevices;
};

class CCamScheduler
{
public:
	CCamScheduler(void);
	~CCamScheduler(void);

	void SetMainWorker(MainWorker *pMainWorker);
/*
	void StartCameraGrabber(MainWorker *pMainWorker);
	void StopCameraGrabber();
*/
    void ReloadCameras();
	std::vector<cameraDevice> GetCameraDevices();

	bool TakeSnapshot(const unsigned long long CamID, std::vector<unsigned char> &camimage);
	bool TakeSnapshot(const std::string &CamID, std::vector<unsigned char> &camimage);
	bool TakeRaspberrySnapshot(std::vector<unsigned char> &camimage);
	bool TakeUVCSnapshot(std::vector<unsigned char> &camimage);
	cameraDevice* GetCamera(const unsigned long long CamID);
	cameraDevice* GetCamera(const std::string &CamID);
	unsigned long long IsDevSceneInCamera(const unsigned char DevSceneType, const unsigned long long DevSceneID);
	unsigned long long IsDevSceneInCamera(const unsigned char DevSceneType, const std::string &DevSceneID);

	bool EmailCameraSnapshot(const std::string &CamIdx, const std::string &subject);
	void ReloadCameraActiveDevices(const std::string &CamID);
	std::string GetCameraURL(cameraDevice *pCamera);
	std::string GetCameraURL(const std::string &CamID);
	std::string GetCameraURL(const unsigned long long CamID);
	std::string GetCameraFeedURL(const std::string &CamID);
	std::string GetCameraFeedURL(const unsigned long long CamID);
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
};

