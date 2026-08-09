#include "stdafx.h"
#include <iostream>
#include <algorithm>
#include "Camera.h"
#include "HTMLSanitizer.h"
#include "Logger.h"
#include "Helper.h"
#include "mainworker.h"
#include "../httpclient/HTTPClient.h"
#include "../smtpclient/SMTPClient.h"
#include <libwebem/Base64.h>
#include "SQLHelper.h"
#include "WebServer.h"
#include <libwebem/cWebem.h>
#include <json/json.h>

#define CAMERA_POLL_INTERVAL 30

namespace
{
	// A camera is only refreshed in the background while the web interface keeps asking for
	// it. Nothing is polled when nobody is looking at a camera, so a system with many
	// configured cameras generates no traffic at all while its dashboards are closed.
	constexpr auto kDemandWindow = std::chrono::seconds(60);

	// Bounds on how often a camera is re-fetched. The actual interval follows the rate at
	// which the web interface asks for that camera, so the background refresh never runs
	// faster than the UI polls it.
	constexpr auto kMinRefreshInterval = std::chrono::seconds(1);
	constexpr auto kMaxRefreshInterval = std::chrono::seconds(30);

	// How long a request waits for the very first frame of a camera that has nothing cached
	// yet, so the first page load still shows a picture. A camera that has already failed
	// carries a backoff deadline and returns immediately instead of waiting again.
	constexpr auto kFirstFrameWait = std::chrono::seconds(3);

	// How many cameras are fetched at the same time, so one slow camera does not hold up
	// the refresh of the others.
	constexpr size_t kMaxParallelFetches = 4;

	// Backoff after a failed fetch: 5s, 10s, 20s, 40s, capped at 60s.
	std::chrono::seconds SnapshotBackoff(int failcount)
	{
		int secs = 5;
		for (int i = 1; i < failcount && secs < 60; i++)
			secs *= 2;
		return std::chrono::seconds(std::min(secs, 60));
	}
} // namespace

extern std::string szUserDataFolder;

CCameraHandler::CCameraHandler()
{
	m_seconds_counter = 0;
}

void CCameraHandler::Start()
{
	m_stoprequested = false;
	m_snapshot_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadName(m_snapshot_thread->native_handle(), "CameraSnapshot");
}

void CCameraHandler::Stop()
{
	if (!m_snapshot_thread)
		return;
	{
		std::lock_guard<std::mutex> l(m_snapshot_mutex);
		m_stoprequested = true;
	}
	m_snapshot_cond.notify_all();
	m_snapshot_thread->join();
	m_snapshot_thread.reset();
}

bool CCameraHandler::IsSnapshotDue(const snapshotCache &cache, clock_t::time_point now) const
{
	if (cache.Fetching)
		return false;
	if (now < cache.RetryAfter)
		return false;
	if (now - cache.LastRequest > kDemandWindow)
		return false;
	if (!cache.HaveImage)
		return true;
	return (now - cache.ImageTime) >= cache.RequestInterval;
}

void CCameraHandler::FetchSnapshot(const uint64_t CamID)
{
	std::vector<unsigned char> camimage;
	bool bOk = TakeSnapshot(CamID, camimage) && !camimage.empty();

	std::lock_guard<std::mutex> l(m_snapshot_mutex);
	auto itt = m_snapshots.find(CamID);
	if (itt == m_snapshots.end())
		return; // camera was removed while we were fetching

	snapshotCache &cache = itt->second;
	cache.Fetching = false;
	cache.Tried = true;

	auto now = clock_t::now();
	if (bOk)
	{
		cache.Image = std::move(camimage);
		cache.HaveImage = true;
		cache.ImageTime = now;
		cache.FailCount = 0;
		cache.RetryAfter = now;
	}
	else
	{
		if (cache.FailCount < 10)
			cache.FailCount++;
		cache.RetryAfter = now + SnapshotBackoff(cache.FailCount);
	}
	m_snapshot_cond.notify_all();
}

void CCameraHandler::Do_Work()
{
	while (!m_stoprequested)
	{
		std::vector<uint64_t> due;
		{
			std::unique_lock<std::mutex> l(m_snapshot_mutex);
			m_snapshot_cond.wait_for(l, std::chrono::milliseconds(500),
						 [this] { return m_stoprequested.load() || m_wake_worker; });
			m_wake_worker = false;
			if (m_stoprequested)
				break;

			auto now = clock_t::now();
			for (auto &snapshot : m_snapshots)
			{
				if (IsSnapshotDue(snapshot.second, now))
				{
					snapshot.second.Fetching = true;
					due.push_back(snapshot.first);
				}
			}
		}

		for (size_t ii = 0; ii < due.size(); ii += kMaxParallelFetches)
		{
			std::vector<std::thread> batch;
			for (size_t jj = ii; jj < due.size() && jj < ii + kMaxParallelFetches; jj++)
				batch.emplace_back([this, CamID = due[jj]] { FetchSnapshot(CamID); });
			for (auto &worker : batch)
				worker.join();
		}
	}
}

bool CCameraHandler::GetSnapshot(const std::string &CamID, std::vector<unsigned char> &camimage)
{
	if (!is_number(CamID))
	{
		_log.Log(LOG_ERROR, "Camera: invalid camera id '%s'", CamID.c_str());
		return false;
	}
	return GetSnapshot(std::stoull(CamID), camimage);
}

bool CCameraHandler::GetSnapshot(const uint64_t CamID, std::vector<unsigned char> &camimage)
{
	// Check the camera exists before creating a cache slot for it, so an unknown idx cannot
	// grow the cache. This must happen before m_snapshot_mutex is taken: ReloadCameras()
	// locks m_mutex and then m_snapshot_mutex, so taking them the other way round here
	// would be a lock order inversion.
	{
		std::lock_guard<std::mutex> l(m_mutex);
		if (GetCamera(CamID) == nullptr)
			return false;
	}

	auto now = clock_t::now();
	std::unique_lock<std::mutex> l(m_snapshot_mutex);

	// Scoped deliberately: the wait below releases the lock, and ReloadCameras() may erase
	// this entry while it is released, so no reference into the map may outlive this block.
	bool bWaitForFirstFrame = false;
	{
		snapshotCache &cache = m_snapshots[CamID];

		// Follow the rate at which this camera is actually being asked for, so the background
		// refresh matches the UI's poll interval instead of running at a fixed rate.
		if (cache.LastRequest.time_since_epoch().count() != 0)
		{
			auto gap = now - cache.LastRequest;
			if (gap < kMinRefreshInterval)
				gap = kMinRefreshInterval;
			else if (gap > kMaxRefreshInterval)
				gap = kMaxRefreshInterval;
			cache.RequestInterval = gap;
		}
		cache.LastRequest = now;

		if (cache.HaveImage)
		{
			// Always answer from the cache, even when the frame is stale. The worker replaces
			// it as soon as the camera responds again; blocking here is what froze the UI.
			camimage = cache.Image;
			return true;
		}

		m_wake_worker = true;
		m_snapshot_cond.notify_all();

		// Nothing cached yet. Wait briefly for the first frame, but only for a camera that
		// has never been fetched, so the first page load still shows a picture. Once a
		// camera has been tried we know whether it answers, and a failing one must never
		// make a request wait again: that is what blocked the web server thread. Only one
		// request waits, so a camera that is slow to answer its first frame cannot collect
		// a queue of them either.
		if (!cache.Tried && !cache.WaitingFirst)
		{
			cache.WaitingFirst = true;
			bWaitForFirstFrame = true;
		}
	}

	if (bWaitForFirstFrame)
	{
		m_snapshot_cond.wait_for(l, kFirstFrameWait, [this, CamID] {
			if (m_stoprequested)
				return true;
			auto itt = m_snapshots.find(CamID);
			return (itt != m_snapshots.end()) && (itt->second.HaveImage || itt->second.Tried);
		});
		auto itw = m_snapshots.find(CamID);
		if (itw != m_snapshots.end())
			itw->second.WaitingFirst = false;
	}

	auto itt = m_snapshots.find(CamID);
	if (itt == m_snapshots.end() || !itt->second.HaveImage)
		return false;
	camimage = itt->second.Image;
	return true;
}

void CCameraHandler::ReloadCameras()
{
	std::vector<std::string> _AddedCameras;
	std::lock_guard<std::mutex> l(m_mutex);
	m_cameradevices.clear();
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT ID, Name, Address, Port, Username, Password, ImageURL, Protocol, AspectRatio FROM Cameras WHERE (Enabled == 1) ORDER BY ID");
	if (!result.empty())
	{
		_log.Log(LOG_STATUS, "Camera: settings (re)loaded");
		for (const auto &sd : result)
		{
			cameraDevice citem;
			citem.ID = std::stoull(sd[0]);
			citem.Name = sd[1];
			citem.Address = sd[2];
			citem.Port = atoi(sd[3].c_str());
			citem.Username = base64_decode(sd[4]);
			citem.Password = base64_decode(sd[5]);
			citem.ImageURL = sd[6];
			citem.Protocol = (eCameraProtocol)atoi(sd[7].c_str());
			citem.AspectRatio = (uint8_t)atoi(sd[8].c_str());
			m_cameradevices.push_back(citem);
			_AddedCameras.push_back(sd[0]);
		}
	}

	for (const auto &camera : _AddedCameras)
	{
		//Get Active Devices/Scenes
		ReloadCameraActiveDevices(camera);
	}

	//Drop cached snapshots of cameras that no longer exist or were disabled
	std::lock_guard<std::mutex> ls(m_snapshot_mutex);
	for (auto itt = m_snapshots.begin(); itt != m_snapshots.end();)
	{
		bool bFound = std::any_of(m_cameradevices.begin(), m_cameradevices.end(),
					  [&itt](const cameraDevice &cam) { return cam.ID == itt->first; });
		itt = bFound ? std::next(itt) : m_snapshots.erase(itt);
	}
}

void CCameraHandler::ReloadCameraActiveDevices(const std::string &CamID)
{
	cameraDevice *pCamera = GetCamera(CamID);
	if (pCamera == nullptr)
		return;
	pCamera->mActiveDevices.clear();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT A.ID, A.DevSceneType, A.DevSceneRowID, B.AspectRatio FROM CamerasActiveDevices AS A, Cameras as B WHERE (A.CameraRowID=='%q') AND (B.ID=='%q') ORDER BY A.ID", CamID.c_str(), CamID.c_str());
	if (!result.empty())
	{
		for (const auto &sd : result)
		{
			cameraActiveDevice aDevice;
			aDevice.ID = std::stoull(sd[0]);
			aDevice.DevSceneType = (unsigned char)atoi(sd[1].c_str());
			aDevice.DevSceneRowID = std::stoull(sd[2]);
			aDevice.AspectRatio = std::stoi(sd[3]);
			pCamera->mActiveDevices.push_back(aDevice);
		}
	}
}

//Return 0 if NO, otherwise Cam IDX
uint64_t CCameraHandler::IsDevSceneInCamera(const unsigned char DevSceneType, const std::string &DevSceneID)
{
	return IsDevSceneInCamera(DevSceneType, std::stoull(DevSceneID));
}

uint64_t CCameraHandler::IsDevSceneInCamera(const unsigned char DevSceneType, const uint64_t DevSceneID)
{
	std::lock_guard<std::mutex> l(m_mutex);
	for (const auto& sd : m_cameradevices)
	{
		for (const auto& sd2 : sd.mActiveDevices)
		{
			if ((sd2.DevSceneType == DevSceneType) && (sd2.DevSceneRowID == DevSceneID))
			{
				return sd.ID;
			}
		}
	}
	return 0;
}

std::string CCameraHandler::GetCameraURL(const std::string &CamID)
{
	cameraDevice* pCamera = GetCamera(CamID);
	if (pCamera == nullptr)
		return "";
	return GetCameraURL(pCamera);
}

std::string CCameraHandler::GetCameraURL(const uint64_t CamID)
{
	cameraDevice* pCamera = GetCamera(CamID);
	if (pCamera == nullptr)
		return "";
	return GetCameraURL(pCamera);
}

std::string CCameraHandler::GetCameraURL(cameraDevice *pCamera)
{
	std::stringstream s_str;

	bool bHaveUPinURL = (pCamera->ImageURL.find("#USERNAME") != std::string::npos) || (pCamera->ImageURL.find("#PASSWORD") != std::string::npos);

	std::string szURLPreFix = (pCamera->Protocol == CPROTOCOL_HTTP) ? "http" : "https";

	if ((!bHaveUPinURL) && ((!pCamera->Username.empty()) || (!pCamera->Password.empty())))
		s_str << szURLPreFix << "://" << CURLEncode::URLEncode(pCamera->Username) << ":" << CURLEncode::URLEncode(pCamera->Password) << "@" << pCamera->Address << ":" << pCamera->Port;
	else
		s_str << szURLPreFix << "://" << pCamera->Address << ":" << pCamera->Port;
	return s_str.str();
}

CCameraHandler::cameraDevice* CCameraHandler::GetCamera(const std::string &CamID)
{
	return GetCamera(std::stoull(CamID));
}

CCameraHandler::cameraDevice* CCameraHandler::GetCamera(const uint64_t CamID)
{
	for (auto &m : m_cameradevices)
	{
		if (m.ID == CamID)
			return &m;
	}
	return nullptr;
}

int CCameraHandler::GetCameraAspectRatio(const std::string& CamIdx)
{
	return GetCameraAspectRatio(std::stoull(CamIdx));
}

int CCameraHandler::GetCameraAspectRatio(const uint64_t &CamID)
{
	std::lock_guard<std::mutex> l(m_mutex);
	for (const auto& m : m_cameradevices)
	{
		if (m.ID == CamID)
			return m.AspectRatio;
	}
	return 0;
}

bool CCameraHandler::TakeSnapshot(const std::string &CamID, std::vector<unsigned char> &camimage)
{
	if (!is_number(CamID))
	{
		_log.Log(LOG_ERROR, "Camera: invalid camera id '%s'", CamID.c_str());
		return false;
	}
	return TakeSnapshot(std::stoull(CamID), camimage);
}

bool CCameraHandler::TakeRaspberrySnapshotRaspiStill(std::vector<unsigned char>& camimage)
{
	std::string raspparams = "-w 800 -h 600 -t 1";
	m_sql.GetPreferencesVar("RaspCamParams", raspparams);

	std::string OutputFileName = szUserDataFolder + "tempcam.jpg";

	//GizMoCuz: Bookwork has replaced this with libcamera-still
	std::string raspistillcmd = "raspistill " + raspparams + " -o " + OutputFileName;
	std::remove(OutputFileName.c_str());

	//Get our image
	int ret = system(raspistillcmd.c_str());
	if (ret != 0)
	{
		_log.Log(LOG_ERROR, "Error executing raspistill command. returned: %d", ret);
		return false;
	}
	//If all went correct, we should have our file
	try
	{
		std::ifstream is(OutputFileName.c_str(), std::ios::in | std::ios::binary);
		if (is)
		{
			if (is.is_open())
			{
				char buf[512];
				while (is.read(buf, sizeof(buf)).gcount() > 0)
					camimage.insert(camimage.end(), buf, buf + (unsigned int)is.gcount());
				is.close();
				std::remove(OutputFileName.c_str());
				return true;
			}
		}
	}
	catch (...)
	{

	}

	return false;
}

bool CCameraHandler::TakeRaspberrySnapshotRPICamStill(std::vector<unsigned char>& camimage)
{
	std::string raspparams = "--width 800 --height 600 -t 1000";
	m_sql.GetPreferencesVar("RaspCamParams", raspparams);

	std::string OutputFileName = szUserDataFolder + "tempcam.jpg";

	//GizMoCuz: Bookwork has replaced this with libcamera-still
	std::string raspistillcmd = "rpicam-still " + raspparams + " -o " + OutputFileName;
	std::remove(OutputFileName.c_str());

	//Get our image
	int ret = system(raspistillcmd.c_str());
	if (ret != 0)
	{
		_log.Log(LOG_ERROR, "Error executing licamera-still command. returned: %d", ret);
		return false;
	}
	//If all went correct, we should have our file
	try
	{
		std::ifstream is(OutputFileName.c_str(), std::ios::in | std::ios::binary);
		if (is)
		{
			if (is.is_open())
			{
				char buf[512];
				while (is.read(buf, sizeof(buf)).gcount() > 0)
					camimage.insert(camimage.end(), buf, buf + (unsigned int)is.gcount());
				is.close();
				std::remove(OutputFileName.c_str());
				return true;
			}
		}
	}
	catch (...)
	{

	}

	return false;
}

bool CCameraHandler::TakeRaspberrySnapshot(std::vector<unsigned char> &camimage)
{
	bool bUseLibCameraStill = file_exist("/bin/rpicam-still");
	if (bUseLibCameraStill)
		return TakeRaspberrySnapshotRPICamStill(camimage);
	else
		return TakeRaspberrySnapshotRaspiStill(camimage);
}

bool CCameraHandler::TakeUVCSnapshot(const std::string &device, std::vector<unsigned char> &camimage)
{
	std::string uvcparams = "-S80 -B128 -C128 -G80 -x800 -y600 -q100";
	m_sql.GetPreferencesVar("UVCParams", uvcparams);

	std::string OutputFileName = szUserDataFolder + "tempcam.jpg";
	std::string nvcmd = "uvccapture " + uvcparams + " -o" + OutputFileName;
	if (!device.empty()) {
		nvcmd += " -d/dev/" + device;
	}
	std::remove(OutputFileName.c_str());

	try
	{
		//Get our image
		int ret = system(nvcmd.c_str());
		if (ret != 0)
		{
			_log.Log(LOG_ERROR, "Error executing uvccapture command. returned: %d", ret);
			return false;
		}
		//If all went correct, we should have our file
		std::ifstream is(OutputFileName.c_str(), std::ios::in | std::ios::binary);
		if (is)
		{
			if (is.is_open())
			{
				char buf[512];
				while (is.read(buf, sizeof(buf)).gcount() > 0)
					camimage.insert(camimage.end(), buf, buf + (unsigned int)is.gcount());
				is.close();
				std::remove(OutputFileName.c_str());
				return true;
			}
		}
	}
	catch (...)
	{

	}
	return false;
}

bool CCameraHandler::TakeSnapshot(const uint64_t CamID, std::vector<unsigned char> &camimage)
{
	// Copy the camera connection details under the lock, then release it before doing any
	// (potentially slow or hanging) network/system I/O. Holding m_mutex across the snapshot
	// fetch serializes all snapshots and - because IsDevSceneInCamera() shares this mutex and
	// is called for every device/scene/light in the getdevices/getscenes/getlightswitches JSON
	// endpoints - freezes the whole web UI whenever a camera becomes unresponsive (issue #6804).
	std::string szURL;
	std::string szUsername;
	std::string szPassword;
	std::string szImageURL;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		cameraDevice *pCamera = GetCamera(CamID);
		if (pCamera == nullptr)
			return false;
		szURL = GetCameraURL(pCamera);
		szUsername = pCamera->Username;
		szPassword = pCamera->Password;
		szImageURL = pCamera->ImageURL;
	}

	szURL += "/" + szImageURL;
	stdreplace(szURL, "#USERNAME", szUsername);
	stdreplace(szURL, "#PASSWORD", szPassword);

	if (szImageURL == "raspberry.cgi")
		return TakeRaspberrySnapshot(camimage);
	if (szImageURL == "uvccapture.cgi")
		return TakeUVCSnapshot(szUsername, camimage);

	std::vector<std::string> ExtraHeaders;
	return HTTPClient::GETBinary(szURL, ExtraHeaders, camimage, 5);
}

std::string WrapBase64(const std::string &szSource, const size_t lsize = 72)
{
	std::string cstring = szSource;
	std::string ret;
	while (cstring.size() > lsize)
	{
		std::string pstring = cstring.substr(0, lsize);
		if (!ret.empty())
			ret += '\n';
		ret += pstring;
		cstring = cstring.substr(lsize);
	}
	if (!cstring.empty())
	{
		ret += '\n' + cstring;
	}
	return ret;
}

bool CCameraHandler::EmailCameraSnapshot(const std::string &CamIdx, const std::string &subject)
{
	int nValue;
	if (!m_sql.GetPreferencesVar("EmailEnabled", nValue))
	{
		return false;//no email setup
	}
	if (!nValue)
		return false; //disabled

	std::string sValue;
	if (!m_sql.GetPreferencesVar("EmailServer", sValue))
	{
		return false;//no email setup
	}
	if (sValue.empty())
	{
		return false;//no email setup
	}
	if (CamIdx.empty())
		return false;

	std::vector<std::string> splitresults;
	StringSplit(CamIdx, ";", splitresults);

	std::string EmailFrom;
	std::string EmailTo;
	std::string EmailServer = sValue;
	int EmailPort = 25;
	std::string EmailUsername;
	std::string EmailPassword;
	int EmailAsAttachment = 0;
	m_sql.GetPreferencesVar("EmailFrom", EmailFrom);
	m_sql.GetPreferencesVar("EmailTo", EmailTo);
	m_sql.GetPreferencesVar("EmailUsername", EmailUsername);
	m_sql.GetPreferencesVar("EmailPassword", EmailPassword);
	m_sql.GetPreferencesVar("EmailPort", EmailPort);
	m_sql.GetPreferencesVar("EmailAsAttachment", EmailAsAttachment);
	std::string htmlMsg =
		"<html>\r\n"
		"<body>\r\n";

	SMTPClient sclient;
	sclient.SetFrom(CURLEncode::URLDecode(EmailFrom));
	sclient.SetTo(CURLEncode::URLDecode(EmailTo));
	sclient.SetCredentials(base64_decode(EmailUsername), base64_decode(EmailPassword));
	sclient.SetServer(CURLEncode::URLDecode(EmailServer), EmailPort);
	sclient.SetSubject(CURLEncode::URLDecode(subject));

	bool bHaveCapturedCamera = false;

	for (const auto & camIt : splitresults)
	{
		std::vector<unsigned char> camimage;

		if (TakeSnapshot(camIt, camimage))
		{
			bHaveCapturedCamera = true;

			std::vector<char> filedata;
			filedata.insert(filedata.begin(), camimage.begin(), camimage.end());
			std::string imgstring;
			imgstring.insert(imgstring.end(), filedata.begin(), filedata.end());
			imgstring = base64_encode(imgstring);
			imgstring = WrapBase64(imgstring);

			htmlMsg +=
				"<img src=\"data:image/jpeg;base64,";
			htmlMsg +=
				imgstring +
				"\">\r\n";
			if (EmailAsAttachment != 0)
				sclient.AddAttachment(imgstring, "snapshot" + camIt + ".jpg");
		}
	}
	if (!bHaveCapturedCamera)
		return false;

	if (EmailAsAttachment == 0)
		sclient.SetHTMLBody(htmlMsg);
	bool bRet = sclient.SendEmail();
	return bRet;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_GetCameras(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights < 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string rused = request::findValue(&req, "used");

			root["status"] = "OK";
			root["title"] = "getcameras";

			std::vector<std::vector<std::string> > result;
			if (rused == "true") {
				result = m_sql.safe_query("SELECT ID, Name, Enabled, Address, Port, Username, Password, ImageURL, Protocol, AspectRatio FROM Cameras WHERE (Enabled=='1') ORDER BY ID ASC");
			}
			else {
				result = m_sql.safe_query("SELECT ID, Name, Enabled, Address, Port, Username, Password, ImageURL, Protocol, AspectRatio FROM Cameras ORDER BY ID ASC");
			}
			if (!result.empty())
			{
				int ii = 0;
				for (const auto &sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Enabled"] = (sd[2] == "1") ? "true" : "false";
					root["result"][ii]["Address"] = sd[3];
					root["result"][ii]["Port"] = atoi(sd[4].c_str());
					root["result"][ii]["Username"] = base64_decode(sd[5]);
					root["result"][ii]["Password"] = base64_decode(sd[6]);
					root["result"][ii]["ImageURL"] = sd[7];
					root["result"][ii]["Protocol"] = atoi(sd[8].c_str());
					root["result"][ii]["AspectRatio"] = atoi(sd[9].c_str());
					ii++;
				}
			}
		}
		void CWebServer::Cmd_GetCamerasUser(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["status"] = "OK";
			root["title"] = "getcameras_user";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Name FROM Cameras WHERE (Enabled=='1') ORDER BY ID ASC");
			if (!result.empty())
			{
				int ii = 0;
				for (const auto &sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					ii++;
				}
			}
		}
		void CWebServer::GetInternalCameraSnapshot(WebEmSession & session, const request& req, reply & rep)
		{
			std::string request_path;
			request_handler::url_decode(req.uri, request_path);

			std::vector<unsigned char> camimage;
			if (request_path.find("raspberry") != std::string::npos)
			{
				if (!m_mainworker.m_cameras.TakeRaspberrySnapshot(camimage)) {
					return;
				}
			}
			else
			{
				if (!m_mainworker.m_cameras.TakeUVCSnapshot("", camimage)) {
					return;
				}
			}
			reply::set_content(&rep, camimage.begin(), camimage.end());
			reply::add_header_attachment(&rep, "snapshot.jpg");
		}

		void CWebServer::GetCameraSnapshot(WebEmSession & session, const request& req, reply & rep)
		{
			std::vector<unsigned char> camimage;
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
			{
				return;
			}
			// Cached: this runs on the single web server thread, which must never block on
			// a camera that does not answer (issue #6804).
			if (!m_mainworker.m_cameras.GetSnapshot(idx, camimage)) {
				return;
			}
			reply::set_content(&rep, camimage.begin(), camimage.end());
			reply::add_header_attachment(&rep, "snapshot.jpg");
		}

		void CWebServer::Cmd_AddCamera(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights < 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string senabled = request::findValue(&req, "enabled");
			std::string address = HTMLSanitizer::Sanitize(request::findValue(&req, "address"));
			std::string sport = request::findValue(&req, "port");
			std::string username = HTMLSanitizer::Sanitize(request::findValue(&req, "username"));
			std::string password = request::findValue(&req, "password");
			std::string timageurl = HTMLSanitizer::Sanitize(request::findValue(&req, "imageurl"));
			int cprotocol = atoi(request::findValue(&req, "protocol").c_str());
			int aspectratio = atoi(request::findValue(&req, "aspectratio").c_str());
			if ((name.empty()) || (address.empty()) || (timageurl.empty()))
				return;

			std::string imageurl;
			if (request_handler::url_decode(timageurl, imageurl))
			{
				imageurl = base64_decode(imageurl);

				int port = atoi(sport.c_str());
				root["status"] = "OK";
				root["title"] = "AddCamera";
				m_sql.safe_query(
					"INSERT INTO Cameras (Name, Enabled, Address, Port, Username, Password, ImageURL, Protocol, AspectRatio) VALUES ('%q',%d,'%q',%d,'%q','%q','%q',%d,%d)",
					name.c_str(),
					(senabled == "true") ? 1 : 0,
					address.c_str(),
					port,
					base64_encode(username).c_str(),
					base64_encode(password).c_str(),
					imageurl.c_str(),
					cprotocol,
					aspectratio
				);
				m_mainworker.m_cameras.ReloadCameras();
			}
		}

		void CWebServer::Cmd_UpdateCamera(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights < 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string senabled = request::findValue(&req, "enabled");
			std::string address = HTMLSanitizer::Sanitize(request::findValue(&req, "address"));
			std::string sport = request::findValue(&req, "port");
			std::string username = HTMLSanitizer::Sanitize(request::findValue(&req, "username"));
			std::string password = request::findValue(&req, "password");
			std::string timageurl = HTMLSanitizer::Sanitize(request::findValue(&req, "imageurl"));
			int cprotocol = atoi(request::findValue(&req, "protocol").c_str());
			int aspectratio = atoi(request::findValue(&req, "aspectratio").c_str());
			if ((name.empty()) || (senabled.empty()) || (address.empty()) || (timageurl.empty()))
				return;

			std::string imageurl;
			if (request_handler::url_decode(timageurl, imageurl))
			{
				imageurl = base64_decode(imageurl);

				int port = atoi(sport.c_str());

				root["status"] = "OK";
				root["title"] = "UpdateCamera";

				m_sql.safe_query(
					"UPDATE Cameras SET Name='%q', Enabled=%d, Address='%q', Port=%d, Username='%q', Password='%q', ImageURL='%q', Protocol=%d, AspectRatio=%d WHERE (ID == '%q')",
					name.c_str(),
					(senabled == "true") ? 1 : 0,
					address.c_str(),
					port,
					base64_encode(username).c_str(),
					base64_encode(password).c_str(),
					imageurl.c_str(),
					cprotocol,
					aspectratio,
					idx.c_str()
				);
				m_mainworker.m_cameras.ReloadCameras();
			}
		}

		void CWebServer::Cmd_DeleteCamera(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights < 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			root["status"] = "OK";
			root["title"] = "DeleteCamera";

			m_sql.DeleteCamera(idx);
			m_mainworker.m_cameras.ReloadCameras();
		}
	} // namespace server
} // namespace http
