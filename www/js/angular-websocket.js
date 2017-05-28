'use strict';

(function () {
    /**
     * @ngdoc provider
     * @name $websocketProvider
     * @module ngWebsocket
     * @description
     * HTML5 WebSocket provider for AngularJS
     */
	function $websocketProvider() {
		var wsp = this;

		wsp.$$config = {
			lazy: false,
			reconnect: true,
			reconnectInterval: 2000,
			mock: false,
			enqueue: false,
			protocols: null
		};

		wsp.$setup = function (cfg) {
			cfg = cfg || {};
			wsp.$$config = angular.extend({}, wsp.$$config, cfg);

			return wsp;
		};

		wsp.$get = ['$http', function ($http) {
			return new $websocketService(wsp.$$config, $http);
		}];
	}

    /**
     * @ngdoc service
     * @name $websocketService
     * @module ngWebsocketService
     * @description
     * HTML5 Websocket service for AngularJS
     */
	function $websocketService(cfg, $http) {
		var wss = this;

		wss.$$websocketList = {};
		wss.$$config = cfg || {};

		wss.$get = function (url) {
			return wss.$$websocketList[url];
		};

		wss.$new = function (cfg) {
			cfg = cfg || {};

			// Url or url + protocols initialization
			if (typeof cfg === 'string') {
				cfg = { url: cfg };

				// url + protocols
				if (arguments.length > 1) {
					if (typeof arguments[1] === 'string' && arguments[1].length > 0) cfg.protocols = [arguments[1]];
					else if (typeof arguments[1] === 'object' && arguments[1].length > 0) cfg.protocols = arguments[1];
				}
			}

			// If the websocket already exists, return that instance
			var ws = wss.$get(cfg.url);
			if (typeof ws === 'undefined') {
				var wsCfg = angular.extend({}, wss.$$config, cfg);

				ws = new $websocket(wsCfg, $http);
				wss.$$websocketList[wsCfg.url] = ws;
			}

			return ws;
		};
	}

    /**
     * @ngdoc class
     * @name $websocket
     * @module ngWebsocket
     * @description
     * HTML5 Websocket wrapper class for AngularJS
     */
	function $websocket(cfg, $http) {
		var me = this;

		if (typeof cfg === 'undefined' || (typeof cfg === 'object' && typeof cfg.url === 'undefined')) throw new Error('An url must be specified for WebSocket');

		me.$$eventMap = {};
		me.$$ws = undefined;
		me.$$reconnectTask = undefined;
		me.$$reconnectCopy = true;
		me.$$queue = [];
		me.$$config = {
			url: undefined,
			lazy: false,
			reconnect: true,
			reconnectInterval: 2000,
			enqueue: false,
			mock: false,
			protocols: null
		};

		me.$$fireEvent = function () {
			var args = [];

			Array.prototype.push.apply(args, arguments);

			var event = args.shift(),
				handlers = me.$$eventMap[event];

			if (typeof handlers !== 'undefined') {
				for (var i = 0; i < handlers.length; i++) {
					if (typeof handlers[i] === 'function') handlers[i].apply(me, args);
				}
			}
		};

		me.$$init = function (cfg) {

			if (cfg.mock) {
				me.$$ws = new $$mockWebsocket(cfg.mock, $http);
			}
			else if (cfg.protocols) {
				me.$$ws = new WebSocket(cfg.url, cfg.protocols);
			}
			else {
				me.$$ws = new WebSocket(cfg.url);
			}

			me.$$ws.onmessage = function (message) {
				try {
					var decoded = JSON.parse(message.data);
					me.$$fireEvent(decoded.event, decoded.data);
					me.$$fireEvent('$message', decoded);
				}
				catch (err) {
					me.$$fireEvent('$message', message.data);
				}
			};

			me.$$ws.onerror = function (error) {
				me.$$fireEvent('$error', error);
			};

			me.$$ws.onopen = function () {
				// Clear the reconnect task if exists
				if (me.$$reconnectTask) {
					clearInterval(me.$$reconnectTask);
					delete me.$$reconnectTask;
				}

				// Flush the message queue
				if (me.$$config.enqueue && me.$$queue.length > 0) {
					while (me.$$queue.length > 0) {
						if (me.$ready()) me.$$send(me.$$queue.shift());
						else break;
					}
				}

				me.$$fireEvent('$open');
			};

			me.$$ws.onclose = function () {
				// Activate the reconnect task
				if (me.$$config.reconnect) {
					me.$$reconnectTask = setInterval(function () {
						if (me.$status() === me.$CLOSED) me.$open();
					}, me.$$config.reconnectInterval);
				}

				me.$$fireEvent('$close');
			};

			return me;
		};

		me.$CONNECTING = 0;
		me.$OPEN = 1;
		me.$CLOSING = 2;
		me.$CLOSED = 3;

		// TODO: it doesn't refresh the view (maybe $apply on something?)
        /*me.$bind = function (event, scope, model) {
         me.$on(event, function (message) {
         model = message;
         scope.$apply();
         });
         };*/

		me.$on = function () {
			var handlers = [];

			Array.prototype.push.apply(handlers, arguments);

			var event = handlers.shift();
			if (typeof event !== 'string' || handlers.length === 0) throw new Error('$on accept two parameters at least: a String and a Function or an array of Functions');

			me.$$eventMap[event] = me.$$eventMap[event] || [];
			for (var i = 0; i < handlers.length; i++) {
				if (me.$$eventMap[event].indexOf(handlers[i]) < 0)
					me.$$eventMap[event].push(handlers[i]);
			}

			return me;
		};

		me.$un = function () {
			var handlers = [], index, eventMap;

			Array.prototype.push.apply(handlers, arguments);

			var event = handlers.shift();

			if (typeof event !== 'string') throw new Error('$un needs a String representing an event.');

			if (handlers.length == 0) {
				if (typeof me.$$eventMap[event] !== 'undefined')
					delete me.$$eventMap[event];
			}
			else {
				for (var i = 0; i < handlers.length; i++) {
					index = me.$$eventMap[event].indexOf(handlers[i]);

					if (index >= 0) {
						me.$$eventMap[event].splice(index, 1);
					}
				}
			}
		};

		me.$$send = function (message) {
			if (me.$ready()) me.$$ws.send(JSON.stringify(message));
			else if (me.$$config.enqueue) me.$$queue.push(message);
		};

		me.$emit = function (event, data) {
			if (typeof event !== 'string') throw new Error('$emit needs two parameter: a String and a Object or a String');

			var message = {
				event: event,
				data: data
			};

			me.$$send(message);

			return me;
		};

		me.$open = function () {
			me.$$config.reconnect = me.$$reconnectCopy;

			if (me.$status() !== me.$OPEN) me.$$init(me.$$config);
			return me;
		};

		me.$close = function () {
			if (me.$status() !== me.$CLOSED) me.$$ws.close();

			if (me.$$reconnectTask) {
				clearInterval(me.$$reconnectTask);
				delete me.$$reconnectTask;
			}

			me.$$config.reconnect = false;

			return me;
		};

		me.$status = function () {
			if (typeof me.$$ws === 'undefined') return me.$CLOSED;
			else return me.$$ws.readyState;
		};

		me.$ready = function () {
			return me.$status() === me.$OPEN;
		};

		me.$mockup = function () {
			return me.$$config.mock;
		};

		// setup
		me.$$config = angular.extend({}, me.$$config, cfg);
		me.$$reconnectCopy = me.$$config.reconnect;

		if (!me.$$config.lazy) me.$$init(me.$$config);

		return me;
	}

	function $$mockWebsocket(cfg, $http) {
		cfg = cfg || {};

		var me = this,
			openTimeout = cfg.openTimeout || 500,
			closeTimeout = cfg.closeTimeout || 1000,
			messageInterval = cfg.messageInterval || 2000,
			fixtures = cfg.fixtures || {},
			messageQueue = [];

		me.CONNECTING = 0;
		me.OPEN = 1;
		me.CLOSING = 2;
		me.CLOSED = 3;

		me.readyState = me.CONNECTING;

		me.send = function (message) {
			if (me.readyState === me.OPEN) {
				messageQueue.push(message);
				return me;
			}
			else throw new Error('WebSocket is already in CLOSING or CLOSED state.');
		};

		me.close = function () {
			if (me.readyState === me.OPEN) {
				me.readyState = me.CLOSING;

				setTimeout(function () {
					me.readyState = me.CLOSED;

					me.onclose();
				}, closeTimeout);
			}

			return me;
		};

		me.onmessage = function () { };
		me.onerror = function () { };
		me.onopen = function () { };
		me.onclose = function () { };

		setInterval(function () {
			if (messageQueue.length > 0) {
				var message = messageQueue.shift(),
					msgObj = JSON.parse(message);

				switch (msgObj.event) {
					case '$close':
						me.close();
						break;
					default:
						// Check for a custom response
						if (typeof fixtures[msgObj.event] !== 'undefined') {
							msgObj.data = fixtures[msgObj.event].data || msgObj.data;
							msgObj.event = fixtures[msgObj.event].event || msgObj.event;
						}

						message = JSON.stringify(msgObj);

						me.onmessage({
							data: message
						});
				}
			}
		}, messageInterval);

		var start = function (fixs) {
			fixs = fixs || {};
			fixs = fixs instanceof Error ? {} : fixs;

			fixtures = fixs;

			setTimeout(function () {
				me.readyState = me.OPEN;
				me.onopen();
			}, openTimeout);
		};

		// Get fixtures from a server or a file if it's a string
		if (typeof fixtures === 'string') {
			$http.get(fixtures)
				.success(start)
				.error(start);
		}
		else start(fixtures);

		return me;
	}

    /**
     * @ngdoc module
     * @name $websocket
     * @module ngWebsocket
     * @description
     * HTML5 WebSocket module for AngularJS
     */
	angular
		.module('ngWebsocket', [])
		.provider('$websocket', $websocketProvider);
})();
