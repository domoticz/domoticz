#pragma once

#include <string>
#include <boost/thread/shared_mutex.hpp>

#include "concurrent_queue.h"
#include "NotificationObserver.h"
#include "StoppableTask.h"

class CNotificationSystem: public StoppableTask
{
public:

	CNotificationSystem(void);
	~CNotificationSystem(void);
	void Notify(const Notification::_eType type, const Notification::_eStatus status, const std::string& eventdata = "");
	bool NotifyWait(const Notification::_eType type, const Notification::_eStatus status, const std::string& eventdata = "");
	bool Register(CNotificationObserver* pNotifier);
	bool Unregister(CNotificationObserver* pNotifier);
	std::string const GetTypeString(const int type);
	std::string const GetStatusString(const int status);
	void Start();
	void Stop();

private:
	struct _tNotificationQueue
	{
		Notification::_eType type;
		Notification::_eStatus status;
		std::string eventdata;
		queue_element_trigger* trigger;
	};
	struct _tNotificationTypeTable
	{
		Notification::_eType type;
		std::string name;
	};
	struct _tNotificationStatusTable
	{
		Notification::_eStatus status;
		std::string name;
	};

	void QueueThread();
	void UnlockNotificationQueueThread();

	volatile bool m_stoprequested;
	boost::shared_mutex m_mutex;
	std::vector<CNotificationObserver*> m_notifiers;
	concurrent_queue<_tNotificationQueue> m_notificationqueue;
	std::shared_ptr<std::thread> m_pQueueThread;

	static const _tNotificationTypeTable typeTable[];
	static const _tNotificationStatusTable statusTable[];
};


