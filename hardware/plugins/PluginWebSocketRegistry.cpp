#include "stdafx.h"

//
// CPluginWebSocketRegistry
//
// Plugin key used: CPlugin::m_PluginKey (hardware/plugins/Plugins.h line ~124).
// This is the XML <plugin key="..."> attribute passed to CPlugin's constructor as
// PluginKey and stored verbatim.  It is constant for all instances of the same plugin
// type (e.g. "MeshCore"), making it suitable as a fan-out channel key.
//

#include "PluginWebSocketRegistry.h"
#include "../../main/Logger.h"

CPluginWebSocketRegistry &CPluginWebSocketRegistry::Get()
{
	static CPluginWebSocketRegistry instance;
	return instance;
}

void CPluginWebSocketRegistry::Register(const std::string &pluginKey, int hwId)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// Remove any stale entry for this hwId (may be under a different key if the
	// hardware was reconfigured, or re-registering after a framework-driven restart).
	auto itRev = m_hwIdToKey.find(hwId);
	if (itRev != m_hwIdToKey.end())
	{
		const std::string &oldKey = itRev->second;
		if (oldKey == pluginKey)
		{
			// Already present under the same key; expected on a framework-driven restart
			// where the previous Unregister was skipped (e.g. plugin re-enabled in-place).
			_log.Debug(DEBUG_NORM, "PluginWebSocketRegistry: HwdID %d re-registered under '%s' (framework restart).", hwId, pluginKey.c_str());
			return;
		}
		// Stale entry under a different key; clean it up.
		auto itOld = m_keyToHwIds.find(oldKey);
		if (itOld != m_keyToHwIds.end())
		{
			itOld->second.erase(hwId);
			if (itOld->second.empty())
				m_keyToHwIds.erase(itOld);
		}
		m_hwIdToKey.erase(itRev);
		_log.Log(LOG_STATUS, "PluginWebSocketRegistry: HwdID %d re-registered from '%s' to '%s'.", hwId, oldKey.c_str(), pluginKey.c_str());
	}

	m_keyToHwIds[pluginKey].insert(hwId);
	m_hwIdToKey[hwId] = pluginKey;

	_log.Debug(DEBUG_NORM, "PluginWebSocketRegistry: HwdID %d registered under '%s' (%d instance(s)).",
		hwId, pluginKey.c_str(), (int)m_keyToHwIds[pluginKey].size());
}

void CPluginWebSocketRegistry::Unregister(int hwId)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto itRev = m_hwIdToKey.find(hwId);
	if (itRev == m_hwIdToKey.end())
		return;

	const std::string key = itRev->second;
	m_hwIdToKey.erase(itRev);

	auto itFwd = m_keyToHwIds.find(key);
	if (itFwd != m_keyToHwIds.end())
	{
		itFwd->second.erase(hwId);
		if (itFwd->second.empty())
			m_keyToHwIds.erase(itFwd);
	}

	_log.Debug(DEBUG_NORM, "PluginWebSocketRegistry: HwdID %d unregistered from '%s'.", hwId, key.c_str());
}

std::vector<int> CPluginWebSocketRegistry::GetInstances(const std::string &pluginKey) const
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::vector<int> result;
	auto it = m_keyToHwIds.find(pluginKey);
	if (it != m_keyToHwIds.end())
		result.assign(it->second.begin(), it->second.end());
	return result;
}

std::string CPluginWebSocketRegistry::GetKeyForHardware(int hwId) const
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto it = m_hwIdToKey.find(hwId);
	return (it != m_hwIdToKey.end()) ? it->second : std::string();
}
