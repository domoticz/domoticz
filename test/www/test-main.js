/**
 * RequireJS test bootstrap for Karma
 * This file configures RequireJS for the test environment and loads all spec files
 */

var allTestFiles = [];
var TEST_REGEXP = /\.spec\.js$/i;

// Get all the test files from Karma
var pathToModule = function(path) {
    return path.replace(/^\/base\//, '').replace(/\.js$/, '');
};

Object.keys(window.__karma__.files).forEach(function(file) {
    if (TEST_REGEXP.test(file)) {
        // Normalize paths to RequireJS module names
        allTestFiles.push(pathToModule(file));
    }
});

require.config({
    // Karma serves files under /base, which is the basePath from karma.conf.js
    baseUrl: '/base/app',

    paths: {
        'angular': '../js/angular.min',
        'angular-mocks': '../js/angular-mocks',
        'angular-route': '../js/angular-route.min',
        'angular-animate': '../js/angular-animate.min',
        'ngSanitize': '../js/angular-sanitize.min',
        'angular-md5': '../js/angular-md5.min',
        'ui-grid': '../js/ui-grid.min',
        'highcharts-ng': '../js/highcharts-ng.min',
        'angularAMD': '../js/angularAMD.min',
        'angular-tree-control': '../js/angular-tree-control',
        'ngDraggable': '../js/ngDraggable',
        'ui.bootstrap': '../js/ui-bootstrap.min',
        'angular.directives-round-progress': '../js/angular-round-progress-directive',
        'angular.scrollglue': '../js/angular-scrollglue',
        'angular-websocket': '../js/angular-websocket',
        'luxon': '../js/luxon.min',
        'lodash': '../js/lodash-custom.min',
        'services/deviceDetection': 'services/deviceDetection'
    },

    shim: {
        'angular': {
            exports: 'angular'
        },
        'angular-mocks': {
            deps: ['angular'],
            exports: 'angular.mock'
        },
        'angularAMD': ['angular'],
        'angular-route': ['angular'],
        'angular-animate': ['angular'],
        'ngSanitize': ['angular'],
        'angular-md5': ['angular'],
        'ui-grid': ['angular'],
        'highcharts-ng': ['angular'],
        'angular-tree-control': ['angular'],
        'ui.bootstrap': ['angular'],
        'ngDraggable': ['angular'],
        'angular.directives-round-progress': ['angular'],
        'angular.scrollglue': ['angular'],
        'angular-websocket': ['angular']
    },

    // dynamically load all test files
    deps: allTestFiles,

    // we have to kickoff jasmine, as it is asynchronous
    callback: window.__karma__.start
});
