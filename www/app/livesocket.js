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

		init();

		return {
			/**
			 * @deprecated prefer to use domoticzApi service instead
			 */
			getJson: getJson,
			sendRequest: sendRequest,
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

			webSocket.$on('$message', handleMessage)
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
					sunset: msg.Sunset
				});
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
