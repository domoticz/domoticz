#pragma once

#include "DomoticzHardware.h"

class CAirconWithMe: public CDomoticzHardwareBase
{
private:
    struct UIDDeviceInfo
    {
        int32_t mUID = 0;
        bool mWritable = false;
        //std::string mRW = "RW";
        int32_t mType = 0;
        std::vector<int32_t> mAvailableStates;
        std::string mSelectorStates;
        std::map<int32_t, int32_t> mSelectorToState;
        std::map<int32_t, int32_t> mStateToSelector;
        int32_t mMinValue = INT_MIN;
        int32_t mMaxValue = INT_MAX;
    };
    
    enum nDomoticzType
    {
        NDT_SWITCH = 1,
        NDT_THERMOSTAT,
        NDT_SELECTORSWITCH,
        NDT_THERMOMETER,
        NDT_NUMBER,
        NDT_HOUR,
        NDT_STRING
    };
    
    struct nStateDescription
    {
        std::string mOptionName;
        int32_t mStateId;
    };
    
    struct UIDinfo
    {
        int32_t mUID;
        std::string mDefaultName;
        nDomoticzType mDomoticzType;
        bool mDomoticzCanChange;
        std::vector<nStateDescription> mAllStates;
    };
    
    struct DeviceInfo
    {
        int32_t mUID;
        std::string mName;
        std::string mDescription;
        nDomoticzType mDomoticzType;
    };
    

public:
	CAirconWithMe(const int id, const std::string& ipaddress, const unsigned short ipport, const std::string& users, const std::string& password);
	virtual ~CAirconWithMe(void);

	bool WriteToHardware(const char* pdata, const unsigned char length) override;

protected:
	virtual bool StartHardware() override;
	virtual bool StopHardware() override;
	void Do_Work();

	bool DoWebRequest(const std::string& postdata, Json::Value& root, std::string& errorMessage, bool loginWhenFailed);
	bool Login();
	bool GetAvailableDataPoints();
	void ComputerSwitchLevelValues();
	bool GetValues();
	bool GetInfo();
	void UpdateDomoticzWithValue(int32_t uid, int32_t value);
	void UpdateSelectorSwitch(const int32_t uid, const int32_t value, const UIDinfo& valueInfo);

	void SendValueToAirco(const int32_t uid, const int32_t value);

private:
    
    std::string mIpAddress;
	uint16_t mIpPort;
	std::string mUsername;
	std::string mPassword;
	int32_t mPollInterval;

	std::shared_ptr<std::thread> m_thread;
	std::string mSessionId;

	std::map<int32_t, UIDDeviceInfo> mDeviceInfo;
    
    std::map<int32_t, UIDinfo> _UIDMap;
    std::vector<DeviceInfo> _DeviceInfo;
    

};
