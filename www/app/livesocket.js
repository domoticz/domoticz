define(['app', 'angular-websocket'], function (app) {

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
	app.service('livesocket', ['$websocket', '$http', '$rootScope', 'notifyBrowser', function ($websocket, $http, $rootScope, notifyBrowser) {
		var socketservice = {};
		socketservice = {
			/* private member variables */
			initialised: false,
			callbackqueue: [],
			websocket: {},
			/* public: getJson method: See description above */
			getJson: function (url, callback_fn) {
				if (!callback_fn) {
					callback_fn = function (data) {
						$rootScope.$broadcast('jsonupdate', data);
					};
				}
				var use_http = !(url.substr(0, 9) == "json.htm?");
				if (use_http) {
					var loc = window.location, http_uri;
					if (loc.protocol === "https:") {
						http_uri = "https:";
					} else {
						http_uri = "http:";
					}
					http_uri += "//" + loc.host;
					http_uri += loc.pathname;
					// get via json http get
					url = http_uri + url;
					$http({
						url: url,
					}).then(function successCallback(response) {
						callback_fn();
					});
				}
				else {
					var settings = {
						url: url,
						success: callback_fn
					};
					settings.context = settings;
					return socketservice.SendAsync(settings);
				}
			},
			/* private method: Initializes the websocket connection if not already done */
			Init: function () {
				if (socketservice.initialised) {
					return;
				}
				var loc = window.location, ws_uri;
				if (loc.protocol === "https:") {
					ws_uri = "wss:";
				} else {
					ws_uri = "ws:";
				}
				ws_uri += "//" + loc.host;
				ws_uri += loc.pathname + "json";
				socketservice.websocket = $websocket.$new({
					url: ws_uri,
					protocols: ["domoticz"],
					lazy: false,
					reconnect: true,
					reconnectInterval: 2000,
					enqueue: true
				});
				socketservice.websocket.callbackqueue = [];
				socketservice.websocket.$on('$open', function () {
					// console.log("websocket opened");
				});
				socketservice.websocket.$on('$close', function () {
					// console.log("websocket closed");
				});
				socketservice.websocket.$on('$error', function () {
					console.log("websocket error");
				});
				socketservice.websocket.$on('$message', function (msg) {
					if (typeof msg == "string") {
						msg = JSON.parse(msg);
					}
					switch (msg.event) {
						case "notification":
							notifyBrowser.notify(msg.Subject, msg.Text);
							return;
						case "date_time":
							$rootScope.SetTimeAndSun(msg.Sunrise, msg.Sunset, msg.ServerTime);
							return;
					}
					var requestid = msg.requestid;
					if (requestid >= 0) {
						var callback_obj = socketservice.callbackqueue[requestid];
						var settings = callback_obj.settings;
						var data = msg.data || msg;
						if (typeof data == "string") {
							data = JSON.parse(data);
						}
						callback_obj.defer_object.resolveWith(settings.context, [settings.success, data]);
					}
					else {
						var data = msg.data || msg;
						if (typeof data == "string") {
							data = JSON.parse(data);
						}
						
						var title = "Devices";
						var broadcast_channel = 'jsonupdate';

						if (msg.request == "scene_request") {
							title = "Scenes";
							broadcast_channel = 'scene_update';
						}
						//alert("title: " + title + "\nreq_id: " + requestid + "\ndata: " + msg.data + ", msg: " + msg + "\n, data: " + JSON.stringify(data));
						var send = {
							title: title,
							item: (typeof data.result != 'undefined') ? data.result[0] : null,
							ServerTime: data.ServerTime,
							Sunrise: data.Sunrise,
							Sunset: data.Sunset
						}
						$rootScope.$broadcast(broadcast_channel, send);
					}
					if (!$rootScope.$$phase) { // prevents triggering a $digest if there's already one in progress
						$rootScope.$digest();
					}
				});
				socketservice.initialised = true;
			},
			/* public: Closes the websocket connection */
			Close: function () {
				if (!socketservice.initialised) {
					return;
				}
				socketservice.websocket.$close();
				socketservice.initialised = false;
			},
			/* public: Sends data through the websocket connection */
			Send: function (data) {
				socketservice.Init();
				socketservice.websocket.$$send(data);
				//socketservice.websocket.$emit('message', data);
			},
			/* This function is not used yet. Todo: check session security in the Domoticz backend. */
			SendLoginInfo: function (sessionid) {
				socketservice.Send(new Blob["2", sessionid]);
			},
			/* mimic ajax call with a deferred object */
			SendAsync: function (settings) {
				socketservice.Init();
				var defer_object = new $.Deferred();
				defer_object.done(function (fn, json) {
					fn.call(this, json);
				});
				var requestid = socketservice.websocket.callbackqueue.length;
				socketservice.callbackqueue.push({ settings: settings, defer_object: defer_object });
				var requestobj = { "event": "request", "requestid": requestid, "query": settings.url.substr(9) };
				socketservice.Send(requestobj);
				return defer_object.promise();
			}
		}
		/* make sure that the websocket gets connection immediately upon connecting to the livesocket angular service */
		socketservice.Init();
		return socketservice;
	}]);

	/* The stub below can be used to override all ajax calls to websocket requests at the same time without changing the other code */
	/*
	var oAjax = $.ajax;
	$.ajax = function (settings) {
		if (settings.url.substr(0, 9) == "json.htm?" && settings.url.match(/type=devices/)) {
			if (typeof settings.context === 'undefined') settings.context = settings;
			return websocket.SendAsync(settings);
		}
		else {
			return oAjax(settings);
		}
	};
	*/
	/* end ajax override */

});
