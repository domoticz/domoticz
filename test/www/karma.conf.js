// Karma configuration for Domoticz AngularJS tests
module.exports = function(config) {
    config.set({
        // Base path that will be used to resolve all patterns (eg. files, exclude)
        basePath: '../../www/',

        // Frameworks to use
        // Available frameworks: https://npmjs.org/browse/keyword/karma-adapter
        frameworks: ['jasmine', 'requirejs'],

        // List of files / patterns to load in the browser
        files: [
            // RequireJS test bootstrap
            { pattern: '../test/www/test-main.js', included: true },

            // Libraries
            { pattern: 'js/angular.min.js', included: false },
            { pattern: 'js/angular-mocks.js', included: false },
            { pattern: 'js/angular-route.min.js', included: false },
            { pattern: 'js/angular-animate.min.js', included: false },
            { pattern: 'js/angular-sanitize.min.js', included: false },
            { pattern: 'js/angularAMD.min.js', included: false },

            // Application files
            { pattern: 'app/**/*.js', included: false },

            // Test specs
            { pattern: '../test/www/unit/**/*.spec.js', included: false }
        ],

        // List of files / patterns to exclude
        exclude: [
            'app/main.js'
        ],

        // Preprocess matching files before serving them to the browser
        // Available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor
        preprocessors: {
            'app/**/*.js': ['coverage']
        },

        // Test results reporter to use
        // Possible values: 'dots', 'progress'
        // Available reporters: https://npmjs.org/browse/keyword/karma-reporter
        reporters: ['progress', 'coverage'],

        // Coverage reporter configuration
        coverageReporter: {
            type: 'html',
            dir: '../test/www/coverage/'
        },

        // Web server port
        port: 9876,

        // Enable / disable colors in the output (reporters and logs)
        colors: true,

        // Level of logging
        // Possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
        logLevel: config.LOG_INFO,

        // Enable / disable watching file and executing tests whenever any file changes
        autoWatch: true,

        // Start these browsers
        // Available browser launchers: https://npmjs.org/browse/keyword/karma-launcher
        browsers: ['ChromeHeadless'],

        // Continuous Integration mode
        // If true, Karma captures browsers, runs the tests and exits
        singleRun: false,

        // Concurrency level
        // How many browser should be started simultaneous
        concurrency: Infinity
    });
};
