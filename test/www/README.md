# Domoticz Frontend Test Suite

This directory contains the unit test infrastructure for the Domoticz AngularJS frontend application.

## Overview

The test suite uses:
- **Jasmine** - BDD test framework
- **Karma** - Test runner
- **RequireJS** - AMD module loading (matching production setup)
- **Chrome Headless** - Browser for running tests

## Setup

1. Install Node.js and npm if not already installed

2. Install test dependencies:
```bash
cd www/
npm install
```

## Running Tests

### Run all tests once
```bash
cd www/
npm test
# or
npm run test:once
```

### Run tests in watch mode (auto-rerun on file changes)
```bash
cd www/
npm run test:watch
```

### Run tests with coverage report
```bash
cd www/
npm run test:coverage
```

Coverage reports are generated in `test/www/coverage/` directory.

## Writing Tests

### Directory Structure
```
test/www/
├── karma.conf.js          # Karma configuration
├── test-main.js           # RequireJS bootstrap for tests
├── unit/                  # Unit test specs
│   ├── dashboard/
│   │   └── dashboardService.spec.js
│   ├── services/
│   │   └── deviceDetection.spec.js
│   └── widgets/
│       ├── dzLightWidget.spec.js
│       ├── dzSceneWidget.spec.js
│       └── dzUtilityWidget.spec.js
└── coverage/              # Generated coverage reports
```

### Test File Convention

Test files should:
- Be placed in `test/www/unit/` mirroring the `www/app/` structure
- Use `.spec.js` suffix
- Use AMD module format with RequireJS

### Example Test Structure

```javascript
define([
    'angular',
    'angular-mocks',
    'path/to/module'
], function() {
    'use strict';

    describe('MyService', function() {
        var myService;

        beforeEach(function() {
            module('domoticz');

            inject(function(_myService_) {
                myService = _myService_;
            });
        });

        it('should do something', function() {
            expect(myService.method()).toBe(true);
        });
    });
});
```

### Mocking Dependencies

Use `$provide` to mock dependencies:

```javascript
beforeEach(function() {
    module('domoticz');

    module(function($provide) {
        $provide.factory('domoticzApi', function($q) {
            return {
                sendRequest: jasmine.createSpy('sendRequest')
                    .and.returnValue($q.resolve({ result: [] }))
            };
        });
    });

    inject(function(_myService_) {
        myService = _myService_;
    });
});
```

### Testing Directives

```javascript
var $compile, $rootScope, $scope, element;

beforeEach(inject(function(_$compile_, _$rootScope_) {
    $compile = _$compile_;
    $rootScope = _$rootScope_;
    $scope = $rootScope.$new();
}));

it('should compile', function() {
    $scope.data = { name: 'test' };
    element = $compile('<my-directive data="data"></my-directive>')($scope);
    $scope.$digest();

    expect(element.html()).toContain('test');
});
```

## Current Test Coverage

The following modules have unit tests:

1. **dashboardService** (`app/dashboard/dashboardService.js`)
   - Device loading and categorization
   - Device/scene switching
   - Evohome operations
   - Real-time update subscriptions

2. **deviceDetection** (`app/services/deviceDetection.js`)
   - Mobile/tablet/desktop detection
   - Override management
   - Server configuration integration

3. **dzLightWidget** (`app/widgets/dzLightWidget.js`)
   - Directive compilation
   - Device type detection
   - Switch operations
   - Icon selection

## CI/CD Integration

To integrate with CI/CD pipelines:

```bash
# Run tests in CI mode (single run, exit code indicates pass/fail)
npm run test:once

# With coverage
npm run test:coverage
```

## Troubleshooting

### Tests not found
- Ensure test files have `.spec.js` suffix
- Check that files are in `test/unit/` directory
- Verify RequireJS paths in `test-main.js`

### Module loading errors
- Check that all dependencies are listed in the `define()` array
- Ensure paths match those in `app/main.js`
- Verify that angular-mocks is loaded

### Chrome not found
Install Chrome or use a different browser in `karma.conf.js`:
```javascript
browsers: ['Firefox', 'ChromeHeadless', 'PhantomJS']
```

## Future Improvements

- Add tests for remaining widgets (dzSceneWidget, dzUtilityWidget)
- Add tests for dashboard controllers
- Add E2E tests with Protractor or Cypress
- Increase code coverage to >80%
- Add visual regression tests
- Integrate with GitHub Actions

## Resources

- [Jasmine Documentation](https://jasmine.github.io/)
- [Karma Documentation](https://karma-runner.github.io/)
- [AngularJS Testing Guide](https://docs.angularjs.org/guide/unit-testing)
- [RequireJS Documentation](https://requirejs.org/)
