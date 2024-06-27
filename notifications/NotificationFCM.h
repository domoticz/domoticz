#pragma once
#include "NotificationBase.h"

class CNotificationFCM : public CNotificationBase {
public:
	CNotificationFCM();
	~CNotificationFCM() override = default;
	bool IsConfigured() override;

      protected:
	bool SendMessageImplementation(uint64_t Idx, const std::string &Name, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound,
				       bool bFromNotification) override;
	  private:
	std::string GAPI_FCM_ProjectID;
	std::string GAPI_FCM_PostURL;
	std::string GAPI_FCM_issuer;
	std::string GAPI_FCM_privkey;

	std::string slAccesToken_cached;
	std::chrono::_V2::system_clock::time_point slAccessToken_exp_time;

	bool getSlAccessToken(const std::string &bearer_token, std::string &slAccessToken);
	bool createFCMjwt(const std::string &FCMissuer, std::string &sFCMjwt);
};
