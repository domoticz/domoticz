#include "stdafx.h"

#ifdef ENABLE_PYTHON

#include "PluginWebSocketBridge.h"
#include "PluginManager.h"
#include "Plugins.h"
#include "PluginMessages.h"
#include "../../main/Logger.h"

namespace Plugins
{
	// Declared in PluginManager.cpp — guards CPluginSystem::m_pPlugins.
	extern std::mutex PluginMutex;

	bool EnqueueWebSocketMessage(int hwId, const std::string &data)
	{
		CPlugin *pPlugin = nullptr;
		{
			CPluginSystem mgr;
			std::map<int, CDomoticzHardwareBase *> *pPlugins = mgr.GetHardware();
			std::lock_guard<std::mutex> l(PluginMutex);

			auto it = pPlugins->find(hwId);
			if (it == pPlugins->end())
			{
				_log.Debug(DEBUG_WEBSERVER, "PluginWebSocketBridge: hwId %d not found, dropping message", hwId);
				return false;
			}

			pPlugin = dynamic_cast<CPlugin *>(it->second);
		}

		if (!pPlugin)
		{
			_log.Debug(DEBUG_WEBSERVER, "PluginWebSocketBridge: hwId %d is not a CPlugin, dropping message", hwId);
			return false;
		}

		// Safe to call onWebSocketMessage() here after releasing PluginMutex: the
		// method only enqueues onto pPlugin's own m_QueueMutex-guarded queue and
		// does not touch the Python C-API from this thread.  pPlugin cannot be
		// deleted while we hold no mutex because device->Stop() in mainworker.cpp
		// joins the plugin worker thread before DeregisterPlugin() / delete runs
		// (~lines 326, 401), ensuring the object outlives any concurrent enqueue.
		pPlugin->onWebSocketMessage(data);
		return true;
	}

} // namespace Plugins

#else // !ENABLE_PYTHON

namespace Plugins
{
	// No-op fallback: Python support is disabled so no plugin instances exist.
	bool EnqueueWebSocketMessage(int /*hwId*/, const std::string & /*data*/)
	{
		return false;
	}
} // namespace Plugins

#endif // ENABLE_PYTHON

