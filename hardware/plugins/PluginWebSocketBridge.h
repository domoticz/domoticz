#pragma once

//
// PluginWebSocketBridge
//
// Python-header-free interface that lets main/ enqueue an inbound websocket
// message onto a specific plugin instance's message queue without ever touching
// the Python C-API from the websocket thread.
//
// main/DomoticzWebsocketHandler.cpp includes only this header; the Python-
// dependent implementation lives in PluginWebSocketBridge.cpp which is compiled
// as part of the plugins module (ENABLE_PYTHON guard applies there).
//

#include <string>

namespace Plugins
{
	// Enqueue an inbound websocket message for the plugin instance identified by
	// hwId.  The call is non-blocking: it locks only the plugin's m_QueueMutex
	// (a short critical section) and returns immediately.
	//
	// Returns true if the message was successfully enqueued, false if the plugin
	// instance was not found or has already stopped (safe to ignore — caller
	// should count successes for the ack reply).
	//
	// data: the JSON-serialised payload string forwarded from the frontend.
	bool EnqueueWebSocketMessage(int hwId, const std::string &data);

} // namespace Plugins
