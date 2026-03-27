#pragma once
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

using SSEEventCallback = std::function<void(const std::string& jsonData)>;

class CPhilipsHueSSE
{
public:
	CPhilipsHueSSE(const std::string& ipAddress,
		const std::string& port,
		const std::string& applicationKey,
		SSEEventCallback callback);
	~CPhilipsHueSSE();

	void Start();
	void Stop();
	bool IsConnected() const;

	CPhilipsHueSSE(const CPhilipsHueSSE&) = delete;
	CPhilipsHueSSE& operator=(const CPhilipsHueSSE&) = delete;

private:
	void ReaderThread();
	bool ConnectAndRead();

	std::string m_IPAddress;
	std::string m_Port;
	std::string m_ApplicationKey;
	SSEEventCallback m_callback;

	std::shared_ptr<std::thread> m_thread;
	std::atomic<bool> m_stop{ false };
	std::atomic<bool> m_connected{ false };
};
