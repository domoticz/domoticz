define(['app'], function (app) {
	app.controller('LogController', ['$scope', '$rootScope', '$http', '$timeout', 'livesocket', 'bootbox', function ($scope, $rootScope, $http, $timeout, livesocket, bootbox) {

		// Constants
		var LOG_NORM = 0x0000001;
		var LOG_STATUS = 0x0000002;
		var LOG_ERROR = 0x0000004;
		var LOG_DEBUG = 0x0000008;
		var LOG_ALL = 0xFFFFFFF;
		var MAX_LOG_LINES = 1000;
		var RENDER_LIMIT_STEP = 200;
		var BATCH_INTERVAL = 100;

		// State
		var nextId = 0;
		var logVersion = 0;
		$scope.logitems = [];
		$scope.filteredItems = [];
		$scope.renderItems = [];
		$scope.renderLimit = RENDER_LIMIT_STEP;
		$scope.hasOlderMessages = false;
		$scope.LastLogTime = 0;
		$scope.searchText = '';

		// Level visibility
		$scope.levelFilters = {
			norm: true,
			status: true,
			error: true,
			debug: true
		};

		// Level counts
		$scope.counts = { norm: 0, status: 0, error: 0, debug: 0 };

		function getLevelClass(level) {
			switch (level) {
				case LOG_STATUS: return 'log-level-status';
				case LOG_ERROR: return 'log-level-error';
				case LOG_DEBUG: return 'log-level-debug';
				default: return 'log-level-norm';
			}
		}

		function getLevelLabel(level) {
			switch (level) {
				case LOG_STATUS: return 'STATUS';
				case LOG_ERROR: return 'ERROR';
				case LOG_DEBUG: return 'DEBUG';
				default: return 'NORM';
			}
		}

		function isLevelVisible(level) {
			switch (level) {
				case LOG_NORM: return $scope.levelFilters.norm;
				case LOG_STATUS: return $scope.levelFilters.status;
				case LOG_ERROR: return $scope.levelFilters.error;
				case LOG_DEBUG: return $scope.levelFilters.debug;
				default: return true;
			}
		}

		function updateCounts(level, delta) {
			switch (level) {
				case LOG_NORM: $scope.counts.norm += delta; break;
				case LOG_STATUS: $scope.counts.status += delta; break;
				case LOG_ERROR: $scope.counts.error += delta; break;
				case LOG_DEBUG: $scope.counts.debug += delta; break;
			}
		}

		// Tiered retention: remove oldest normal/debug entries first
		function trimLogBuffer() {
			while ($scope.logitems.length > MAX_LOG_LINES) {
				var idx = -1;
				for (var i = 0; i < $scope.logitems.length; i++) {
					if ($scope.logitems[i].level !== LOG_ERROR && $scope.logitems[i].level !== LOG_STATUS) {
						idx = i;
						break;
					}
				}
				if (idx === -1) idx = 0;
				var removed = $scope.logitems.splice(idx, 1)[0];
				updateCounts(removed.level, -1);
			}
		}

		// Add a log entry (handles multiline messages)
		$scope.addLogEntry = function (level, message) {
			var lines = message.replace(/\n/g, '\n').split('\n');
			if (lines.length < 1) return;

			var firstParts = lines[0].split(' ');
			if (firstParts.length < 3) return;

			var sdate = firstParts[0];
			var stime = firstParts[1];

			for (var i = 0; i < lines.length; i++) {
				if (!lines[i]) continue;
				var lineMsg = (i === 0) ? lines[i] : sdate + ' ' + stime + ' ' + lines[i];

				// Parse timestamp from message
				var match = lineMsg.match(/^(\S+\s+\S+)\s+(.*)$/);
				var timestamp = match ? match[1] : '';
				var text = match ? match[2] : lineMsg;

				$scope.logitems.push({
					id: nextId++,
					timestamp: timestamp,
					level: level,
					levelClass: getLevelClass(level),
					levelLabel: getLevelLabel(level),
					text: text,
					raw: lineMsg
				});

				updateCounts(level, 1);
			}

			trimLogBuffer();
			logVersion++;
		};

		// Compute filtered items
		function updateFilteredItems() {
			var searchRegex = null;
			var searchLower = '';

			if ($scope.searchText) {
				var regexMatch = $scope.searchText.match(/^\/(.+)\/([gimsuy]*)$/);
				if (regexMatch) {
					try {
						searchRegex = new RegExp(regexMatch[1], regexMatch[2]);
					} catch (e) {
						searchRegex = null;
					}
				}
				if (!searchRegex) {
					searchLower = $scope.searchText.toLowerCase();
				}
			}

			$scope.filteredItems = $scope.logitems.filter(function (item) {
				if (!isLevelVisible(item.level)) return false;
				if ($scope.searchText) {
					if (searchRegex) {
						return searchRegex.test(item.raw);
					}
					return item.raw.toLowerCase().indexOf(searchLower) !== -1;
				}
				return true;
			});

			updateRenderItems();
		}

		// Render cap: only put last N filtered items into the DOM
		function updateRenderItems() {
			var total = $scope.filteredItems.length;
			var start = Math.max(0, total - $scope.renderLimit);
			$scope.renderItems = $scope.filteredItems.slice(start);
			$scope.hasOlderMessages = start > 0;
		}

		$scope.showOlderMessages = function () {
			$scope.renderLimit += RENDER_LIMIT_STEP;
			updateRenderItems();
		};

		// Watch for filter changes — reset render window
		$scope.$watchGroup(['searchText', 'levelFilters.norm', 'levelFilters.status',
			'levelFilters.error', 'levelFilters.debug'], function () {
			$scope.renderLimit = RENDER_LIMIT_STEP;
			updateFilteredItems();
		});

		// Update when new items are added (version changes even when length stays the same at capacity)
		$scope.$watch(function () { return logVersion; }, function (newVal, oldVal) {
			if (newVal !== oldVal) {
				updateFilteredItems();
				scrollIfNeeded();
			}
		});

		// Initial load via HTTP
		function loadInitialLogs() {
			$http({
				url: 'json.htm?type=command&param=getlog&lastlogtime=0&loglevel=' + LOG_ALL
			}).then(function (response) {
				var data = response.data;
				if (data.result) {
					if (data.LastLogTime) {
						$scope.LastLogTime = parseInt(data.LastLogTime);
					}
					data.result.forEach(function (item) {
						$scope.addLogEntry(item.level, item.message);
					});
				}
			});
		}

		// Clear log
		$scope.ClearLog = function () {
			bootbox.confirm('Are you sure you want to clear the log?').then(function () {
				livesocket.unsubscribeFrom('log');
				$http({
					url: 'json.htm?type=command&param=clearlog'
				}).then(function () {
					$scope.logitems = [];
					$scope.filteredItems = [];
					$scope.renderItems = [];
					$scope.hasOlderMessages = false;
					$scope.renderLimit = RENDER_LIMIT_STEP;
					$scope.counts = { norm: 0, status: 0, error: 0, debug: 0 };
					$scope.LastLogTime = 0;
					livesocket.subscribeTo('log');
				}, function () {
					livesocket.subscribeTo('log');
				});
			}, angular.noop);
		};

		// Batched WebSocket listener
		var pendingMessages = [];
		var batchTimer = null;

		var unsubLog = $scope.$on('log', function (event, data) {
			pendingMessages.push({ level: data.level, message: data.message });
			if (!batchTimer) {
				batchTimer = $timeout(function () {
					pendingMessages.forEach(function (msg) {
						$scope.addLogEntry(msg.level, msg.message);
					});
					pendingMessages = [];
					batchTimer = null;
				}, BATCH_INTERVAL);
			}
		});

		// Format log items as plain text
		function formatLogText(items) {
			return items.map(function (item) {
				return item.raw;
			}).join('\n');
		}

		// Safe digest helper
		function safeApply(fn) {
			if ($scope.$$phase || $rootScope.$$phase) {
				fn();
			} else {
				$scope.$apply(fn);
			}
		}

		// Get text selected by the user in the log console, if any
		function getSelectedLogText() {
			var selection = window.getSelection();
			if (!selection || selection.isCollapsed) return '';
			var logEl = document.getElementById('logdata');
			if (!logEl) return '';
			var range = selection.getRangeAt(0);
			if (logEl.contains(range.commonAncestorContainer)) {
				return selection.toString().trim();
			}
			return '';
		}

		// Copy filtered log to clipboard
		$scope.copyFeedback = false;
		$scope.copyToClipboard = function () {
			var selectedText = getSelectedLogText();
			var text = selectedText || formatLogText($scope.filteredItems);
			if (!text) return;

			if (navigator.clipboard && navigator.clipboard.writeText) {
				navigator.clipboard.writeText(text).then(function () {
					safeApply(function () { $scope.copyFeedback = true; });
					setTimeout(function () {
						safeApply(function () { $scope.copyFeedback = false; });
					}, 2000);
				});
			} else {
				var textarea = document.createElement('textarea');
				textarea.value = text;
				textarea.style.position = 'fixed';
				textarea.style.opacity = '0';
				document.body.appendChild(textarea);
				textarea.select();
				document.execCommand('copy');
				document.body.removeChild(textarea);
				$scope.copyFeedback = true;
				setTimeout(function () {
					safeApply(function () { $scope.copyFeedback = false; });
				}, 2000);
			}
		};

		// Download filtered log as .txt file
		$scope.downloadLog = function () {
			var text = formatLogText($scope.filteredItems);
			if (!text) return;

			var timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
			var filename = 'domoticz-log-' + timestamp + '.txt';
			var blob = new Blob([text], { type: 'text/plain;charset=utf-8' });
			var url = URL.createObjectURL(blob);
			var a = document.createElement('a');
			a.href = url;
			a.download = filename;
			a.click();
			URL.revokeObjectURL(url);
		};

		// Auto-scroll state
		$scope.autoScroll = true;
		var logContainer = null;

		function initScrollHandler() {
			logContainer = document.getElementById('logdata');
			if (!logContainer) return;

			logContainer.addEventListener('scroll', function () {
				if (ignoreScrollEvents) return;
				var atBottom = (logContainer.scrollHeight - logContainer.scrollTop - logContainer.clientHeight) < 20;
				if ($scope.autoScroll !== atBottom) {
					$scope.autoScroll = atBottom;
					safeApply(function () {});
				}
			});
		}

		var ignoreScrollEvents = false;

		$scope.scrollToBottom = function () {
			if (!logContainer) return;
			ignoreScrollEvents = true;
			$scope.renderLimit = RENDER_LIMIT_STEP;
			updateRenderItems();
			logContainer.scrollTop = logContainer.scrollHeight;
			$scope.autoScroll = true;
			$timeout(function () {
				ignoreScrollEvents = false;
			});
		};

		function scrollIfNeeded() {
			if ($scope.autoScroll && logContainer) {
				ignoreScrollEvents = true;
				requestAnimationFrame(function () {
					if (logContainer) {
						logContainer.scrollTop = logContainer.scrollHeight;
					}
					ignoreScrollEvents = false;
				});
			}
		}

		// Init - defer to allow DOM to render
		$timeout(function () {
			initScrollHandler();
			loadInitialLogs();
			livesocket.subscribeTo('log');
		});

		// Cleanup
		$scope.$on('$destroy', function () {
			if (batchTimer) $timeout.cancel(batchTimer);
			unsubLog();
			livesocket.unsubscribeFrom('log');
		});
	}]);
});
