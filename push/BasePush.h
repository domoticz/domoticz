#pragma once

#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/signals2.hpp>
#include "../main/StoppableTask.h"
#include <mutex>

class CBasePush : public StoppableTask
{
public:
	enum class PushType
	{
		PUSHTYPE_UNKNOWN = 0,
		PUSHTYPE_FIBARO,
		PUSHTYPE_GOOGLE_PUB_SUB,
		PUSHTYPE_HTTP,
		PUSHTYPE_INFLUXDB,
		PUSHTYPE_WEBSOCKET
	};
	struct _tPushLinks
	{
		uint64_t DeviceRowIdx;
		std::string DeviceName;
		int DelimiterPos;
		int devType;
		int devSubType;
		int metertype;
		PushType pushType;
	};

	CBasePush();

	static std::vector<std::string> DropdownOptions(const int devType, const int devSubType);
	static std::string DropdownOptionsValue(const int devType, const int devSubType, const int pos);
	static std::string ProcessSendValue(const uint64_t DeviceRowIdx, const std::string& rawsendValue, int delpos, int nValue, int includeUnit, int devType, int devSubType, int metertype);

	void ReloadPushLinks(const PushType PType);
	bool GetPushLink(const uint64_t DeviceRowIdx, _tPushLinks& plink);

protected:
	PushType m_PushType;
	bool m_bLinkActive;
	boost::signals2::connection m_sConnection;
	boost::signals2::connection m_sDeviceUpdate;
	boost::signals2::connection m_sNotification;
	boost::signals2::connection m_sSceneChanged;

	static std::string getUnit(const int devType, const int devSubType, const int delpos, const int metertypein);

	static unsigned long get_tzoffset();
	static std::string get_lastUpdate(uint64_t);

	static void replaceAll(std::string& context, const std::string& from, const std::string& to);

	bool IsLinkInDatabase(const uint64_t DeviceRowIdx);

	std::mutex m_link_mutex;

private:
	std::vector<_tPushLinks> m_pushlinks;
};

