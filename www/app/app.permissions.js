define(['angular'], function () {
    var module = angular.module('domoticz.permissions', []);

    module.factory('permissions', function ($rootScope) {
        var permissionList;
        return {
            setPermissions: function (permissions) {
                permissionList = permissions;
                window.my_config = {
                    userrights: permissionList.rights
                };
                $rootScope.$broadcast('permissionsChanged');
            },
            hasPermission: function (permission) {
                if (permission === 'Admin') {
                    return (permissionList.rights == 2);
                }
                if (permission === 'User') {
                    return (permissionList.rights >= 0);
                }
                if (permission === 'Viewer') {
                    return (permissionList.rights == 0);
                }
                alert('Unknown permission request: ' + permission);
                return false;
            },
            hasLogin: function (isloggedin) {
                return (permissionList.isloggedin == isloggedin);
            },
            isAuthenticated: function () {
                return (permissionList.rights != -1);
            }
        };
    });

    module.directive('hasPermission', function (permissions) {
        return {
            link: function (scope, element, attrs) {
                var value = attrs.hasPermission.trim();
                var notPermissionFlag = value[0] === '!';
                if (notPermissionFlag) {
                    value = value.slice(1).trim();
                }

                function toggleVisibilityBasedOnPermission() {
                    var hasPermission = permissions.hasPermission(value);
                    if (hasPermission && !notPermissionFlag || !hasPermission && notPermissionFlag)
                        element.show();
                    else
                        element.hide();
                }

                toggleVisibilityBasedOnPermission();
                scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
            }
        };
    });

    module.directive('hasLogin', function (permissions) {
        return {
            link: function (scope, element, attrs) {
                var bvalue = (attrs.hasLogin === 'true');

                function toggleVisibilityBasedOnPermission() {
                    if (permissions.hasLogin(bvalue))
                        element.show();
                    else
                        element.hide();
                }

                toggleVisibilityBasedOnPermission();
                scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
            }
        };
    });

    module.directive('hasLoginNoAdmin', function (permissions) {
        return {
            link: function (scope, element, attrs) {
                function toggleVisibilityBasedOnPermission() {
                    var bVisible = !permissions.hasPermission('Admin');
                    if (bVisible) {
                        bVisible = permissions.hasLogin(true);
                    }
                    if (bVisible == true)
                        element.show();
                    else
                        element.hide();
                }

                toggleVisibilityBasedOnPermission();
                scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
            }
        };
    });

    module.directive('hasUser', function (permissions) {
        return {
            link: function (scope, element, attrs) {
                function toggleVisibilityBasedOnPermission() {
                    if (permissions.isAuthenticated())
                        element.show();
                    else
                        element.hide();
                }

                toggleVisibilityBasedOnPermission();
                scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
            }
        };
    });

    return module;
});
