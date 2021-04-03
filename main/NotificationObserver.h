#pragma once

class CNotificationObserver
{
public:
	enum _eType : uint16_t
	{
		DZ_START,               // 0
		DZ_STOP,                // 1
		DZ_BACKUP_DONE,         // 2
		DZ_NOTIFICATION,        // 3
		HW_TIMEOUT,             // 4
		HW_START,               // 5
		HW_STOP,                // 6
		HW_THREAD_ENDED,        // 7
		DZ_CUSTOM,              // 8
		DZ_ALLEVENTRESET,       // 9
		DZ_ALLDEVICESTATUSRESET // 10
	};

	enum _eStatus : uint8_t
	{
		STATUS_OK,
		STATUS_INFO,
		STATUS_ERROR,
		STATUS_WARNING
	};

	CNotificationObserver() = default;
	~CNotificationObserver() = default;
	virtual bool Update(const _eType type, const _eStatus status, const std::string &eventdata = "") = 0;
};

typedef CNotificationObserver Notification;
