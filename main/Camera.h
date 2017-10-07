#pragma once

#include <string>
#include <vector>

class CCameraHandler
{
	enum eCameraProtocol
	{
		CPROTOCOL_HTTP = 0,
		CPROTOCOL_HTTPS,
	};

	struct cameraActiveDevice
	{
		uint64_t ID;
		uint64_t DevSceneRowID;
		unsigned char DevSceneType;
	};

	struct cameraDevice
	{
		uint64_t ID;
		std::string Name;
		std::string Address;
		std::string Username;
		std::string Password;
		eCameraProtocol Protocol;
		int Port;
		std::string ImageURL;
		std::vector<cameraActiveDevice> mActiveDevices;
	};
public:
	CCameraHandler(void);
	~CCameraHandler(void);

    void ReloadCameras();

	bool TakeSnapshot(const uint64_t CamID, std::vector<unsigned char> &camimage);
	bool TakeSnapshot(const std::string &CamID, std::vector<unsigned char> &camimage);
	bool TakeRaspberrySnapshot(std::vector<unsigned char> &camimage);
	bool TakeUVCSnapshot(const std::string &device, std::vector<unsigned char> &camimage);
	cameraDevice* GetCamera(const uint64_t CamID);
	cameraDevice* GetCamera(const std::string &CamID);
	uint64_t IsDevSceneInCamera(const unsigned char DevSceneType, const uint64_t DevSceneID);
	uint64_t IsDevSceneInCamera(const unsigned char DevSceneType, const std::string &DevSceneID);

	bool EmailCameraSnapshot(const std::string &CamIdx, const std::string &subject);
	std::string GetCameraURL(cameraDevice *pCamera);
	std::string GetCameraURL(const std::string &CamID);
	std::string GetCameraURL(const uint64_t CamID);
private:
	void ReloadCameraActiveDevices(const std::string &CamID);

	boost::mutex m_mutex;
	unsigned char m_seconds_counter;
	std::vector<cameraDevice> m_cameradevices;
};

