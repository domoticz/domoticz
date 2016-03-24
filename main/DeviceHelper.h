#pragma once
#include <boost/shared_ptr.hpp>
#include "../hardware/hardwaretypes.h"
#include "Device.h"

namespace Devices
{
	class CDeviceHelper
	{
	public:
		CDeviceHelper();
		~CDeviceHelper();

		void	Unload();
		size_t	Load();

		CSensorPtr FindDevice(DeviceID DomoticzID);
		void	AddDevice(DeviceID DomoticzID);
		void	UpdateDevice(DeviceID DomoticzID);
		void	DeepUpdateDevice(DeviceID DomoticzID);
		void	DeleteDevice(DeviceID DomoticzID);

		CSensorPtr FindScene(DeviceID DomoticzID);
		void	AddScene(DeviceID DomoticzID);
		void	UpdateScene(DeviceID DomoticzID);
		void	DeepUpdateScene(DeviceID DomoticzID);
		void	DeleteScene(DeviceID DomoticzID);

		CDeviceMapPtr Find(_eDeviceCategoryMask Category);
		CDeviceMapPtr Find(int Roomplan);
		CDeviceMapPtr Find(int Roomplan, _eDeviceCategoryMask Category);
		CDeviceMapPtr Find(std::vector<DeviceID> Devices);

	private:
		size_t	LoadDevices();
		void	LoadDevice(DeviceID	DomoticzID, _eSwitchType sType, int dType, int subType);
		size_t	LoadScenes();
		void	LoadScene(DeviceID	DomoticzID, _eSceneGroupType sType);

		boost::shared_mutex m_devicelistMutex;
		CDeviceMapPtr m_devicelist;
	};
}