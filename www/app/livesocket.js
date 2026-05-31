define(['app.notifications', 'angular-websocket'], function (appNotificationsModule) {

	var module = angular.module('domoticz.websocket', ['ngWebsocket', appNotificationsModule.name]);

	/*
		The livesocket service connects a websocket to domoticz in the Init() method.
		This socket connection stays live through all the interface page life.
		Via the websocket, notifications are pushed by Domoticz sending a msg.event == 'notification' object.
		Furthermore, get requests can be issued by the getJson(url, callback_fn) method. The url will be the same as if
		passed through an ajax call. The callback function can also be the same, ergo this function is designed to replace the usual ajax requests.
		An added feature is that devices that are retrieved via the json call, also from then on get real time status updates via a broadcast message.
		These status updates can be updated in a live manner. Example (taken from UtilityController.js):
			$scope.$on('jsonupdate', function (event, data) {
				RefreshItem(data.item);
			});
		With this, periodic ajax requests are not neccesary anymore. As the moment there is a device update, the new information gets broadcasted
		immediately.		
	*/
	module.service('livesocket', function ($websocket, $http, $rootScope, $q, $location, notifyBrowser) {
		var webSocket;
		var requestsCount = 0;
		var requestsQueue = [];
		var pluginListeners = {};
		var activePluginTopics = {};

		init();

		return {
			getJson: getJson,
			sendRequest: sendRequest,
			subscribeTo: subscribeTo,
			unsubscribeFrom: unsubscribeFrom,
			unsubscribeDevices: unsubscribeDevices,
			subscribePlugin: subscribePlugin,
			unsubscribePlugin: unsubscribePlugin,
			sendPluginCommand: sendPluginCommand,
			onPluginMessage: onPluginMessage
		};

		function init() {
			var wsProtocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
			var wsURI = wsProtocol + '//' + location.host + location.pathname + 'json';

			webSocket = $websocket.$new({
				url: wsURI,
				protocols: ["domoticz"],
				lazy: false,
				reconnect: true,
				reconnectInterval: 2000,
				enqueue: true
			});

			webSocket.$on('$message', handleMessage);

			webSocket.$on('$open', function () {
				// Re-subscribe through subscribeTo so reconnect goes through the same
				// requestid/ack path as initial subscriptions (via subscribePlugin).
				// subscribeTo is safe here: the socket is open, $$send is available, and
				// handleRequestResponse (registered on $message) will resolve the promise
				// when the ack arrives.  No recursion risk — $open does not call $open.
				Object.keys(activePluginTopics).forEach(function (name) {
					subscribeTo('plugin:' + name);
				});
			});
		}

		function handleMessage(msg) {
			if (typeof msg == "string") {
				msg = JSON.parse(msg);
			}

			switch (msg.event) {
				case "notification":
					notifyBrowser.notify(msg.Subject, msg.Text);
					return;
				case "date_time":
					handleTimeUpdate(msg);
					return;
				case "log":
					handleLog(msg);
					return;
				case "plugin":
					handlePluginMessage(msg);
					return;
			}

			if (msg.requestid >= 0) {
				handleRequestResponse(msg);
			} else {
				var data = msg.data ? JSON.parse(msg.data) : msg;

				if (msg.request === 'device_request' && data.status === 'OK') {
					if (typeof data.result !== 'undefined') {
						data.result.forEach(function(device) {
							$rootScope.$broadcast('device_update', device);
						});
					}
					handleTimeUpdate(data);
				}

				if (msg.request === 'scene_request' && data.status === 'OK') {
					if (typeof data.result !== 'undefined') {
						data.result.forEach(function(item) {
							$rootScope.$broadcast('scene_update', item);
						});
					}
					handleTimeUpdate(data);
				}
			}

			if (!$rootScope.$$phase) {
				$rootScope.$digest();
			}
		}

		function handleRequestResponse(msg) {
			var requestIndex = requestsQueue.findIndex(function (item) {
				return item.requestId === msg.requestid;
			});

			if (requestIndex === -1) {
				return;
			}

			var requestInfo = requestsQueue[requestIndex];
			var payload = msg.data ? JSON.parse(msg.data) : msg;
			requestInfo.callback(payload);
			requestsQueue.splice(requestIndex, 1);
		}

		function handleTimeUpdate(msg) {
			if (typeof msg.ServerTime !== 'undefined') {
				$rootScope.$broadcast('time_update', {
					serverTime: msg.ServerTime,
					sunrise: msg.Sunrise,
					sunset: msg.Sunset,
					actTime: msg.ActTime
				});
				if (!$rootScope.$$phase) {
					$rootScope.$digest();
				}
			}
		}

		function handleLog(msg) {
			if (typeof msg.message !== 'undefined') {
				$rootScope.$broadcast('log', {
					level: msg.level,
					message: msg.message
				});
			}
		}

		function handlePluginMessage(msg) {
			var name = msg.plugin;
			if (typeof name === 'undefined') {
				return;
			}
			var payload = { plugin: name, hwid: msg.hwid, data: msg.data };
			// Deliver only via the callback registry (onPluginMessage).
			// $rootScope.$broadcast was removed to avoid double-dispatch; all
			// consumers should register via onPluginMessage instead.
			var listeners = pluginListeners[name];
			if (listeners) {
				// Defensive copy so an unsubscribe inside a callback does not
				// corrupt the iteration (fix-4).
				listeners.slice().forEach(function (cb) {
					try {
						cb(payload);
					} catch (e) {
						console.error('[livesocket] plugin listener threw for "' + name + '":', e);
					}
				});
			}
			// Trigger a digest so data-bound UI reacts to the pushed payload,
			// consistent with handleTimeUpdate's own explicit $digest pattern.
			if (!$rootScope.$$phase) {
				$rootScope.$digest();
			}
		}

		function getJson(url, callback_fn) {
			if (!callback_fn) {
				callback_fn = function (data) {
					$rootScope.$broadcast('jsonupdate', data);
				};
			}
			var use_http = !(url.substr(0, 9) == "json.htm?");

			if (use_http) {
				$http({
					url: url,
				}).then(function(response) {
					callback_fn(response);
				});
			} else {
				return sendRequest(url.substr(9)).then(callback_fn);
			}
		}

		function sendRequest(url) {
			return $q(function (resolve, reject) {
				var requestId = ++requestsCount;

				var requestobj = {
					event: "request",
					requestid: requestId,
					query: url
				};

				var requestInfo = {
					requestId: requestId,
					callback: resolve
				};

				requestsQueue.push(requestInfo);
				webSocket.$$send(requestobj);
			});
		}
		
		function subscribeTo(topic) {
			return $q(function (resolve, reject) {
				var requestId = ++requestsCount;

				var requestobj = {
					event: "subscribe",
					requestid: requestId,
					topic: topic
				};

				var requestInfo = {
					requestId: requestId,
					callback: resolve
				};

				requestsQueue.push(requestInfo);
				webSocket.$$send(requestobj);
			});
		}
		function unsubscribeFrom(topic) {
			return $q(function (resolve, reject) {
				var requestId = ++requestsCount;

				var requestobj = {
					event: "unsubscribe",
					requestid: requestId,
					topic: topic
				};

				var requestInfo = {
					requestId: requestId,
					callback: resolve
				};

				requestsQueue.push(requestInfo);
				webSocket.$$send(requestobj);
			});
		}

		function unsubscribeDevices() {
			webSocket.$$send({ event: 'unsubscribe_devices' });
		}

		/**
		 * Subscribe to plugin push messages by name.
		 * The subscription is automatically restored on websocket reconnect.
		 *
		 * @param {string} name - Plugin name (matches the `plugin` field in pushed frames).
		 * @returns {Promise} Resolves with the server ack when the subscribe frame is confirmed.
		 */
		function subscribePlugin(name) {
			if (activePluginTopics[name]) {
				return $q.when();
			}
			activePluginTopics[name] = true;
			return subscribeTo('plugin:' + name);
		}

		/**
		 * Cancel a plugin subscription by name.
		 * Future pushes for this plugin name will not be delivered.
		 *
		 * @param {string} name - Plugin name to unsubscribe.
		 * @returns {Promise} Resolves with the server ack.
		 */
		function unsubscribePlugin(name) {
			delete activePluginTopics[name];
			return unsubscribeFrom('plugin:' + name);
		}

		/**
		 * Send a command to a plugin.
		 * If hwid is supplied it must be an integer; if not, the call is rejected
		 * with a console error and nothing is sent.
		 * When hwid is omitted the key is not included in the frame.
		 *
		 * @param {string} name      - Plugin name.
		 * @param {*}      data      - Arbitrary payload forwarded to the plugin.
		 * @param {number} [hwid]    - Optional hardware-instance id (integer).
		 */
		function sendPluginCommand(name, data, hwid) {
			if (typeof hwid !== 'undefined') {
				if (typeof hwid !== 'number' || hwid !== Math.floor(hwid)) {
					console.error('[livesocket] sendPluginCommand: hwid must be an integer, got:', hwid);
					return;
				}
			}
			var msg = { event: 'plugin_command', plugin: name, data: data };
			if (typeof hwid !== 'undefined') {
				msg.hwid = hwid;
			}
			var serialized = JSON.stringify(msg);
			if (serialized.length > 65536) {
				console.error('[livesocket] sendPluginCommand: payload too large (' + serialized.length + ' bytes); limit is 65536 bytes. Message not sent.');
				return;
			}
			webSocket.$$send(msg);
		}

		/**
		 * Register a callback for inbound plugin push messages.
		 * Subscriptions are auto-restored on reconnect when subscribePlugin was used.
		 *
		 * @param {string}   name - Plugin name to listen for.
		 * @param {Function} cb   - Called with `{plugin, hwid, data}` on each push.
		 * @returns {Function} Unbind function — call it to remove this listener.
		 */
		function onPluginMessage(name, cb) {
			if (!pluginListeners[name]) {
				pluginListeners[name] = [];
			}
			pluginListeners[name].push(cb);
			return function () {
				var arr = pluginListeners[name];
				if (!arr) { return; }
				var idx = arr.indexOf(cb);
				if (idx !== -1) {
					arr.splice(idx, 1);
					// Reclaim the slot when the last listener is removed (fix-1).
					if (arr.length === 0) {
						delete pluginListeners[name];
					}
				}
			};
		}


	});

	/* The stub below can be used to override all ajax calls to websocket requests at the same time without changing the other code */
	/*
	var oAjax = $.ajax;
	$.ajax = function (settings) {
		if (settings.url.substr(0, 9) == "json.htm?" && settings.url.match(/param=getdevices/)) {
			if (typeof settings.context === 'undefined') settings.context = settings;
			return websocket.SendAsync(settings);
		}
		else {
			return oAjax(settings);
		}
	};
	*/
	/* end ajax override */

	return module;
});
