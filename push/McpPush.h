#pragma once
#include "BasePush.h"
#include "../main/lsignal.h"
#include "../main/StoppableTask.h"
#include <boost/signals2.hpp>
#include <mutex>

enum _eLogLevel : uint32_t;

class CMcpPush : public CBasePush, public StoppableTask
{
public:
	CMcpPush();
	~CMcpPush();
	void Start();
	void Stop();
	void onDeviceTableChanged();
	static CMcpPush& Instance();

private:
	void OnDeviceReceived(int hwdID, uint64_t DeviceRowIdx,
	                      const std::string& DeviceName,
	                      const unsigned char* pRXCommand);
	void OnDeviceUpdate(int hwdID, uint64_t DeviceRowIdx);
	void OnSceneChange(uint64_t SceneIdx, const std::string& SceneName);
	void OnNotificationReceived(const std::string& Subject, const std::string& Text,
	                            const std::string& ExtraData, int Priority,
	                            const std::string& Sound, bool bFromNotification);
	void OnLogMessage(const _eLogLevel level, const std::string& sLogline);

	std::recursive_mutex m_mutex;
	bool m_isStarted = false;

	boost::signals2::connection m_connDeviceReceived;
	boost::signals2::connection m_connDeviceUpdate;
	boost::signals2::connection m_connSceneChanged;
	boost::signals2::connection m_connNotification;
	lsignal::slot m_sLogMessage;
};

extern CMcpPush g_McpPush;
