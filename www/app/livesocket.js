define(['app', 'angular-websocket'], function (app) {

	app.service('livesocket', ['$websocket', '$http', '$rootScope', 'notifyBrowser', function ($websocket, $http, $rootScope, notifyBrowser) {
		var socketservice = {};
		socketservice = {
			initialised: false,
			callbackqueue: [],
			websocket: {},
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
						//alert("req_id: " + requestid + "\ndata: " + msg.data + ", msg: " + msg + "\n, data: " + JSON.stringify(data));
						var send = {
							title: "Devices", // msg.title
							item: (typeof data.result != 'undefined') ? data.result[0] : null,
							ServerTime: data.ServerTime,
							Sunrise: data.Sunrise,
							Sunset: data.Sunset
						}
						$rootScope.$broadcast('jsonupdate', send);
					}
					if (!$rootScope.$$phase) { // prevents triggering a $digest if there's already one in progress
						$rootScope.$digest();
					}
				});
				socketservice.initialised = true;
			},
			Close: function () {
				if (!socketservice.initialised) {
					return;
				}
				socketservice.websocket.$close();
				socketservice.initialised = false;
			},
			Send: function (data) {
				socketservice.Init();
				socketservice.websocket.$$send(data);
				//socketservice.websocket.$emit('message', data);
			},
			SendLoginInfo: function (sessionid) {
				socketservice.Send(new Blob["2", sessionid]);
			},
			/* mimic ajax call */
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
		socketservice.Init();
		return socketservice;
	}]);

	/* this doesnt run, for some reason */
	app.run(['livesocket', function (livesocket) {
		console.log(livesocket);
		alert('run');
		livesocket.Init();
	}]);
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