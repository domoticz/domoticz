#pragma once

#include <string>
#include <vector>

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
	std::string ImageURL;
	std::vector<cameraActiveDevice> mActiveDevices;
};

class CCameraHandler
{
public:
	CCameraHandler(void);
	~CCameraHandler(void);

    void ReloadCameras();

	bool TakeSnapshot(const unsigned long long CamID, std::vector<unsigned char> &camimage);
	bool TakeSnapshot(const std::string &CamID, std::vector<unsigned char> &camimage);
	bool TakeRaspberrySnapshot(std::vector<unsigned char> &camimage);
	bool TakeUVCSnapshot(std::vector<unsigned char> &camimage);
	cameraDevice* GetCamera(const unsigned long long CamID);
	cameraDevice* GetCamera(const std::string &CamID);
	unsigned long long IsDevSceneInCamera(const unsigned char DevSceneType, const unsigned long long DevSceneID);
	unsigned long long IsDevSceneInCamera(const unsigned char DevSceneType, const std::string &DevSceneID);

	bool EmailCameraSnapshot(const std::string &CamIdx, const std::string &subject);
	std::string GetCameraURL(cameraDevice *pCamera);
	std::string GetCameraURL(const std::string &CamID);
	std::string GetCameraURL(const unsigned long long CamID);
private:
	void ReloadCameraActiveDevices(const std::string &CamID);

	boost::mutex m_mutex;
	unsigned char m_seconds_counter;
	std::vector<cameraDevice> m_cameradevices;
};

