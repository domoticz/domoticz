#include "stdafx.h"
#include "PhilipsHueSSE.h"
#include "../../main/Logger.h"
#include <curl/curl.h>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <memory>

#ifdef _DEBUG
//#define DEBUG_PhilipsHue_R
//#define DEBUG_PhilipsHue_W
#endif

#ifdef DEBUG_PhilipsHue_W
extern void SaveString2Disk(std::string str, std::string filename);
#endif

extern std::string urlToFilename(const std::string& prefix, const std::string& url);

struct SSEContext
{
	std::string lineBuf;
	std::string currentData;
	SSEEventCallback callback;
	std::atomic<bool>* stop;
};

static int SSEProgressCallback(void* clientp, curl_off_t /*dltotal*/, curl_off_t /*dlnow*/, curl_off_t /*ultotal*/, curl_off_t /*ulnow*/)
{
	auto* stop = static_cast<std::atomic<bool>*>(clientp);
	return stop->load() ? 1 : 0;
}

static size_t SSEWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	if (!ptr || !userdata)
		return 0;
	auto* ctx = static_cast<SSEContext*>(userdata);
	if (ctx->stop->load())
		return 0;

	size_t total = size * nmemb;
	ctx->lineBuf.append(ptr, total);

	size_t pos;
	while ((pos = ctx->lineBuf.find('\n')) != std::string::npos)
	{
		std::string line = ctx->lineBuf.substr(0, pos);
		ctx->lineBuf.erase(0, pos + 1);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line.empty())
		{
			if (!ctx->currentData.empty())
			{
				try { ctx->callback(ctx->currentData); }
				catch (const std::exception& e) {
					_log.Log(LOG_ERROR, "PhilipsHue SSE: event callback exception: %s", e.what());
				}
#ifdef DEBUG_PhilipsHue_W
				SaveString2Disk("data: " + ctx->currentData + "\n\n",
					urlToFilename("PhilipsHue", "eventstream_clip_v2"));
#endif
				ctx->currentData.clear();
			}
		}
		else if (line.rfind("data:", 0) == 0)
		{
			std::string data = line.substr(5);
			if (!data.empty() && data[0] == ' ')
				data.erase(0, 1);
			if (!ctx->currentData.empty())
				ctx->currentData += '\n';
			ctx->currentData += data;
		}
		// Ignore "id:" and "event:" lines
	}
	return total;
}

CPhilipsHueSSE::CPhilipsHueSSE(const std::string& ipAddress,
	const std::string& port,
	const std::string& applicationKey,
	SSEEventCallback callback)
	: m_IPAddress(ipAddress)
	, m_Port(port)
	, m_ApplicationKey(applicationKey)
	, m_callback(callback)
{
}

CPhilipsHueSSE::~CPhilipsHueSSE()
{
	Stop();
}

void CPhilipsHueSSE::Start()
{
	m_stop = false;
	m_thread = std::make_shared<std::thread>(&CPhilipsHueSSE::ReaderThread, this);
}

void CPhilipsHueSSE::Stop()
{
	m_stop = true;
	if (m_thread && m_thread->joinable())
		m_thread->join();
	m_thread.reset();
}

bool CPhilipsHueSSE::IsConnected() const
{
	return m_connected.load();
}

bool CPhilipsHueSSE::ConnectAndRead()
{
#ifdef DEBUG_PhilipsHue_R
	{
		std::string filename = urlToFilename("PhilipsHue", "eventstream_clip_v2");
		std::ifstream f(filename);
		if (!f.is_open())
		{
			_log.Log(LOG_ERROR, "PhilipsHue SSE: DEBUG_PhilipsHue_R: failed to open %s", filename.c_str());
			return false;
		}
		std::string line, currentData;
		while (std::getline(f, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (line.rfind("data:", 0) == 0)
			{
				currentData = line.substr(5);
				if (!currentData.empty() && currentData[0] == ' ')
					currentData.erase(0, 1);
			}
			else if (line.empty() && !currentData.empty())
			{
				try { m_callback(currentData); }
				catch (const std::exception& e) {
					_log.Log(LOG_ERROR, "PhilipsHue SSE: event callback exception: %s", e.what());
				}
				currentData.clear();
			}
		}
		return false;
	}
#endif

	CURL* curl = curl_easy_init();
	if (!curl)
		return true;

	std::string url = "https://" + m_IPAddress + ":" + m_Port + "/eventstream/clip/v2";

	SSEContext ctx;
	ctx.callback = m_callback;
	ctx.stop = &m_stop;

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, ("hue-application-key: " + m_ApplicationKey).c_str());
	headers = curl_slist_append(headers, "Accept: text/event-stream");

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, SSEWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, SSEProgressCallback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &m_stop);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

	m_connected = true;
	_log.Log(LOG_STATUS, "PhilipsHue SSE: connected to event stream");
	CURLcode res = curl_easy_perform(curl);
	m_connected = false;

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (m_stop.load())
		return false;

	_log.Log(LOG_STATUS, "PhilipsHue SSE: disconnected (%s), reconnecting...",
		curl_easy_strerror(res));
	return true;
}

void CPhilipsHueSSE::ReaderThread()
{
	int reconnectDelay = 1;
	while (!m_stop.load())
	{
		bool shouldReconnect = ConnectAndRead();
		if (!shouldReconnect || m_stop.load())
			break;
		for (int i = 0; i < reconnectDelay * 10 && !m_stop.load(); ++i)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		reconnectDelay = std::min(reconnectDelay * 2, 60);
	}
}
