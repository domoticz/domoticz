define(['angular'], function () {
    var module = angular.module('domoticz.notifications', []);

    module.run(function($window) {
        if ($window.Notification && $window.Notification.permission !== "granted") {
            $window.Notification.requestPermission();
        }
    });

    /*
		The notifyBrowser service sets and gets the browser notifications coming on from the domoticz websocket.
		In livesocket.js the notifications are pushed to the message queue by using the notify() method.
		Each notification times out in 2 seconds.
		The alertarea directive reads the messagequeue using the messages() method.
	*/
    module.service('notifyBrowser', function ($timeout, $window) {
        var messagequeue = [];

        return {
            notify: function (title, body) {
                var item = {"title": title, "body": body, "time": new Date()};
                messagequeue.push(item);
                $timeout(function () {
                    var index = messagequeue.indexOf(item);
                    if (index >= 0) {
                        messagequeue.splice(index, 1);
                    }
                }, 2000);
                if (typeof $window.Notification == "undefined") {
                    console.log("Notification: " + title + ": " + body);
                    console.log('Desktop notifications not available in your browser. Try Chromium.');
                    return;
                }

                if ($window.Notification.permission !== "granted")
                    $window.Notification.requestPermission();
                else {
                    var notification = new $window.Notification(title, {
                        //icon: 'http://cdn.sstatic.net/stackexchange/img/logos/so/so-icon.png',
                        body: body,
                    });
                    var href = $window.location.href;

                    notification.onclick = function () {
                        window.open(href);
                    };
                }
            },
            messages: function () {
                return messagequeue;
            }
        };
    });

    /*
		The alertarea is on every page. It displays the browser notifications.
		It uses the notifyBrowser service to collect the pending notifications.
		Todo: Display them below each other instead of left to each other.
		Todo: Make a "dropdown" css effect
	*/
    module.directive('alertarea', function () {
        return {
            template: '<div class="alerts" style="position: absolute; right: 10px;" ng-shw="messages.length>0"><div class="alert alert-{{(m.type)||\'info\'}} alert-dismissable fade in pull-right" ng-repeat="m in messages"><button type="button" class="close" data-dismiss="alert">×</button><label>{{m.title}}</label><div>{{m.body}}</div></div></div>',
            restrict: "A",
            scope: {},
            controller: function ($scope, notifyBrowser) {
                $scope.messages = [];
                $scope.messages = notifyBrowser.messages();
                $scope.$watch(notifyBrowser.messages, function (messages) {
                    $scope.messages = messages;
                });
            }
        };
    });

    return module
});