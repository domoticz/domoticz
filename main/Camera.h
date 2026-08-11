#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>

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
		uint8_t AspectRatio;
	};

	struct cameraDevice
	{
		uint64_t ID;
		std::string Name;
		std::string Address;
		std::string Username;
		std::string Password;
		eCameraProtocol Protocol;
		uint8_t AspectRatio;
		int Port;
		std::string ImageURL;
		std::vector<cameraActiveDevice> mActiveDevices;
	};
public:
  CCameraHandler();
  ~CCameraHandler() = default;

  void Start();
  void Stop();

  void ReloadCameras();

  // Cached, non-blocking snapshot access. This is what the web server must use: it never
  // performs network I/O on the calling thread, so an unreachable camera can no longer
  // stall every other request (issue #6804).
  bool GetSnapshot(const uint64_t CamID, std::vector<unsigned char> &camimage);
  bool GetSnapshot(const std::string &CamID, std::vector<unsigned char> &camimage);

  // Direct fetch. Blocks the calling thread for as long as the camera takes to answer
  // (up to the HTTP timeout), so it must only be called from a thread that can afford to
  // wait: the snapshot worker below, or the notification/e-mail path.
  bool TakeSnapshot(const uint64_t CamID, std::vector<unsigned char> &camimage);
  bool TakeSnapshot(const std::string &CamID, std::vector<unsigned char> &camimage);
  bool TakeRaspberrySnapshot(std::vector<unsigned char> &camimage);
  bool TakeUVCSnapshot(const std::string &device, std::vector<unsigned char> &camimage);
  cameraDevice *GetCamera(const uint64_t CamID);
  cameraDevice *GetCamera(const std::string &CamID);
  uint64_t IsDevSceneInCamera(const unsigned char DevSceneType, const uint64_t DevSceneID);
  uint64_t IsDevSceneInCamera(const unsigned char DevSceneType, const std::string &DevSceneID);

  bool EmailCameraSnapshot(const std::string &CamIdx, const std::string &subject);
  std::string GetCameraURL(cameraDevice *pCamera);
  std::string GetCameraURL(const std::string &CamID);
  std::string GetCameraURL(const uint64_t CamID);
  int GetCameraAspectRatio(const std::string& CamIdx);
  int GetCameraAspectRatio(const uint64_t &CamID);

private:
	using clock_t = std::chrono::steady_clock;

	// One cache slot per camera the web interface has asked for.
	struct snapshotCache
	{
		std::vector<unsigned char> Image;
		clock_t::time_point ImageTime;	 // when Image was fetched
		clock_t::time_point LastRequest; // when the web interface last asked for this camera
		clock_t::time_point RetryAfter;	 // failure backoff deadline
		clock_t::duration RequestInterval{ std::chrono::seconds(5) };
		int FailCount{ 0 };
		bool Fetching{ false };
		bool HaveImage{ false };
		bool Tried{ false };	   // a fetch has completed at least once, successful or not
		bool WaitingFirst{ false }; // a request is already waiting for the first frame
	};

	void ReloadCameraActiveDevices(const std::string &CamID);
	bool TakeRaspberrySnapshotRaspiStill(std::vector<unsigned char>& camimage);
	bool TakeRaspberrySnapshotRPICamStill(std::vector<unsigned char>& camimage);

	void Do_Work();
	void FetchSnapshot(uint64_t CamID);
	// Caller must hold m_snapshot_mutex.
	bool IsSnapshotDue(const snapshotCache &cache, clock_t::time_point now) const;

	std::mutex m_mutex;
	unsigned char m_seconds_counter;
	std::vector<cameraDevice> m_cameradevices;

	std::mutex m_snapshot_mutex;
	std::condition_variable m_snapshot_cond;
	std::map<uint64_t, snapshotCache> m_snapshots;
	std::shared_ptr<std::thread> m_snapshot_thread;
	std::atomic_bool m_stoprequested{ false };
	bool m_wake_worker{ false };
};

