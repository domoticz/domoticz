#include "stdafx.h"
#include "NotificationSystem.h"
#include "NotificationObserver.h"
#include "Logger.h"
#include "Helper.h"
#include <boost/thread.hpp>
#include "NotificationSystem.h"

const CNotificationSystem::_tNotificationTypeTable CNotificationSystem::typeTable[] =
{ // don't change order
	{ Notification::DZ_START,                "start"                },
	{ Notification::DZ_STOP,                 "stop"                 },
	{ Notification::DZ_BACKUP_DONE,          "backupDone"           },
	{ Notification::DZ_NOTIFICATION,         "notification"         },
	{ Notification::HW_TIMEOUT,              "hardwareTimeout"      },
	{ Notification::HW_START,                "hardwareStart"        },
	{ Notification::HW_STOP,                 "hardwareStop"         },
	{ Notification::HW_THREAD_ENDED,         "threadEnded"          },
	{ Notification::DZ_CUSTOM,               "customEvent"          },
	{ Notification::DZ_ALLEVENTRESET,        "resetAllEvents"       },
	{ Notification::DZ_ALLDEVICESTATUSRESET, "resetAllDeviceStatus" },
};

const CNotificationSystem::_tNotificationStatusTable CNotificationSystem::statusTable[] =
{ // don't change order
	{ Notification::STATUS_OK,             "ok"              },
	{ Notification::STATUS_INFO,           "info"            },
	{ Notification::STATUS_ERROR,          "error"           },
	{ Notification::STATUS_WARNING,        "warning"         }
};

CNotificationSystem::~CNotificationSystem()
{
	Stop();
}

void CNotificationSystem::Start()
{
	RequestStart();
	m_pQueueThread = std::make_shared<std::thread>([this] { QueueThread(); });
	SetThreadName(m_pQueueThread->native_handle(), "NotificationSystemQueue");
}

void CNotificationSystem::Stop()
{
	RequestStop();
	if (m_pQueueThread)
	{
		UnlockNotificationQueueThread();
		m_pQueueThread->join();
		m_pQueueThread.reset();
	}
}

void CNotificationSystem::UnlockNotificationQueueThread()
{
	// Push dummy message to unlock queue
	_tNotificationQueue item;
	item.trigger = nullptr;
	m_notificationqueue.push(item);
}

std::string CNotificationSystem::GetTypeString(const int type)
{
	if (type < sizeof(typeTable) / sizeof(typeTable[0]))
		return typeTable[type].name;
	return "unknown";
}

std::string CNotificationSystem::GetStatusString(const int status)
{
	if (status < sizeof(statusTable) / sizeof(statusTable[0]))
		return statusTable[status].name;
	return "unknown";
}

void CNotificationSystem::QueueThread()
{
	_log.Log(LOG_STATUS, "NotificationSystem: thread started...");

	_tNotificationQueue item;
	while (!IsStopRequested(0))
	{
		bool hasPopped = m_notificationqueue.timed_wait_and_pop<std::chrono::duration<int> >(item, std::chrono::duration<int>(5)); // timeout after 5 sec

		if (!hasPopped)
			continue;

		boost::unique_lock<boost::shared_mutex> lock(m_mutex);
		for (auto &m_notifier : m_notifiers)
			m_notifier->Update(item.type, item.status, item.eventdata);
	}

	m_notificationqueue.clear();
	_log.Log(LOG_STATUS, "NotificationSystem: thread stopped...");
}

void CNotificationSystem::Notify(const Notification::_eType type, const Notification::_eStatus status, const std::string& eventdata)
{
	_tNotificationQueue item;
	item.type = type;
	item.status = status;
	item.eventdata = eventdata;
	m_notificationqueue.push(item);
}

bool CNotificationSystem::NotifyWait(const Notification::_eType type, const Notification::_eStatus status, const std::string& eventdata)
{
	bool response = false;
	boost::unique_lock<boost::shared_mutex> lock(m_mutex);
	for (auto &m_notifier : m_notifiers)
		response |= m_notifier->Update(type, status);
	return response;
}

bool CNotificationSystem::Register(CNotificationObserver* pNotifier)
{
	if (pNotifier == nullptr)
		return false;

	boost::unique_lock<boost::shared_mutex> lock(m_mutex);
	for (auto &m_notifier : m_notifiers)
		if (m_notifier == pNotifier)
			return false;

	m_notifiers.push_back(pNotifier);
	return true;
}

bool CNotificationSystem::Unregister(CNotificationObserver* pNotifier)
{
	if (pNotifier == nullptr)
		return false;

	if (!m_notifiers.empty())
	{
		boost::unique_lock<boost::shared_mutex> lock(m_mutex);
		for (size_t i = 0; i < m_notifiers.size(); i++)
		{
			if (m_notifiers[i] == pNotifier)
			{
				m_notifiers.erase(m_notifiers.begin() + i);
				return true;
			}
		}
	}
	return false;
}
