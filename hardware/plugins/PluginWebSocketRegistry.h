#pragma once

//
// CPluginWebSocketRegistry
//
// Singleton that maps a stable plugin key (CPlugin::m_PluginKey, the XML <plugin key="...">
// attribute, e.g. "MyPlugin") to the set of currently-running HardwareIDs for that plugin
// type.  The reverse map lets an Unregister(hwId) call work without knowing the key.
//
// Thread-safe: all public methods acquire the internal mutex.
//
// Kept Python-header-free so main/ code (F3 inbound routing) can include this without
// pulling the Python C-API headers.
//

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class CPluginWebSocketRegistry
{
public:
	static CPluginWebSocketRegistry &Get();

	// Register a running plugin instance.  If hwId is already present under any key
	// (stale entry from a previous run) it is removed first, then added under pluginKey.
	void Register(const std::string &pluginKey, int hwId);

	// Remove a plugin instance by HardwareID from whatever key currently holds it.
	// No-op if hwId is not found; this is expected when Start() failed before reaching
	// Register() (so the hwId was never registered) or after a clean shutdown.
	void Unregister(int hwId);

	// Return all HardwareIDs currently registered under pluginKey.
	std::vector<int> GetInstances(const std::string &pluginKey) const;

	// Return the plugin key that hwId is currently registered under, or "" if not found.
	std::string GetKeyForHardware(int hwId) const;

private:
	CPluginWebSocketRegistry() = default;
	~CPluginWebSocketRegistry() = default;
	CPluginWebSocketRegistry(const CPluginWebSocketRegistry &) = delete;
	CPluginWebSocketRegistry &operator=(const CPluginWebSocketRegistry &) = delete;

	mutable std::mutex m_mutex;

	// pluginKey -> set of active HardwareIDs for that plugin type
	std::map<std::string, std::set<int>> m_keyToHwIds;

	// reverse: HardwareID -> pluginKey
	std::map<int, std::string> m_hwIdToKey;
};
