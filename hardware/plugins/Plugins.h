#pragma once

#include "../DomoticzHardware.h"
#include "../hardwaretypes.h"
#include "../../notifications/NotificationBase.h"
#include "PythonObjects.h"

#ifndef byte
typedef unsigned char byte;
#endif

namespace Plugins {

	class CDirectiveBase;
	class CEventBase;
	class CPluginMessageBase;
	class CPluginNotifier;
	class CPluginTransport;

	enum PluginDebugMask
	{
		PDM_NONE = 0,
		// 1 is mapped to PDM_ALL in code for backwards compatibility
		PDM_PYTHON = 2,
		PDM_PLUGIN = 4,
		PDM_DEVICE = 8,
		PDM_CONNECTION = 16,
		PDM_IMAGE = 32,
		PDM_MESSAGE = 64,
		PDM_QUEUE = 128,
		PDM_ALL = 65535
	};

	class CPlugin : public CDomoticzHardwareBase
	{
	private:
		int				m_iPollInterval;

		void*			m_PyInterpreter;
		void*			m_PyModule;

		std::string		m_Version;
		std::string		m_Author;

		CPluginNotifier*	m_Notifier;

		std::mutex	m_TransportsMutex;
		std::vector<CPluginTransport*>	m_Transports;

		std::shared_ptr<std::thread> m_thread;

		bool StartHardware() override;
		void Do_Work();
		bool StopHardware() override;
		void ClearMessageQueue();

		void LogPythonException();
		void LogPythonException(const std::string &);

	public:
	  CPlugin(int HwdID, const std::string &Name, const std::string &PluginKey);
	  ~CPlugin() override;

	  int PollInterval(int Interval = -1);
	  void *PythonModule()
	  {
		  return m_PyModule;
	  };
	  void Notifier(const std::string &Notifier = "");
	  void AddConnection(CPluginTransport *);
	  void RemoveConnection(CPluginTransport *);

	  bool Initialise();
	  bool LoadSettings();
	  bool Start();
	  void ConnectionProtocol(CDirectiveBase *);
	  void ConnectionConnect(CDirectiveBase *);
	  void ConnectionListen(CDirectiveBase *);
	  void ConnectionRead(CPluginMessageBase *);
	  void ConnectionWrite(CDirectiveBase *);
	  void ConnectionDisconnect(CDirectiveBase *);
	  void DisconnectEvent(CEventBase *);
	  void Callback(const std::string &sHandler, void *pParams);
	  void RestoreThread();
	  void ReleaseThread();
	  void Stop();

	  void WriteDebugBuffer(const std::vector<byte> &Buffer, bool Incoming);

	  bool WriteToHardware(const char *pdata, unsigned char length) override;
	  void SendCommand(int Unit, const std::string &command, int level, _tColor color);
	  void SendCommand(int Unit, const std::string &command, float level);

	  void onDeviceAdded(int Unit);
	  void onDeviceModified(int Unit);
	  void onDeviceRemoved(int Unit);
	  void MessagePlugin(CPluginMessageBase *pMessage);
	  void DeviceAdded(int Unit);
	  void DeviceModified(int Unit);
	  void DeviceRemoved(int Unit);

	  bool HasNodeFailed(int Unit);

	  std::string m_PluginKey;
	  void *m_DeviceDict;
	  void *m_ImageDict;
	  void *m_SettingsDict;
	  std::string m_HomeFolder;
	  PluginDebugMask m_bDebug;
	  bool m_bIsStarting;
	  bool m_bTracing;
	};

	class CPluginNotifier : public CNotificationBase
	{
	private:
		CPlugin*	m_pPlugin;
	public:
		CPluginNotifier(CPlugin* pPlugin, const std::string & );
		~CPluginNotifier() override;
		bool IsConfigured() override;
		std::string	 GetIconFile(const std::string &ExtraData);
		std::string GetCustomIcon(std::string & szCustom);
	protected:
	  bool SendMessageImplementation(uint64_t Idx, const std::string &Name, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority,
					 const std::string &Sound, bool bFromNotification) override;
	};

	//
//	Holds per plugin state details, specifically plugin object, read using PyModule_GetState(PyObject *module)
//
	struct module_state {
		CPlugin* pPlugin;
		PyObject* error;
	};

} // namespace Plugins
