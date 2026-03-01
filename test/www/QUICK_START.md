# Quick Start Guide - Domoticz Frontend Testing

## 5-Minute Setup

### 1. Install Dependencies
```bash
cd s:/Domoticz/www
npm install
```

### 2. Run Tests
```bash
npm test
```

That's it! Tests will run in Chrome Headless and display results in the terminal.

## Common Commands

```bash
# Run tests once and exit
npm test

# Run tests in watch mode (auto-rerun on file changes)
npm run test:watch

# Run tests with coverage report
npm run test:coverage
```

## Quick Test Template

Create a new test file in `test/unit/[category]/myModule.spec.js`:

```javascript
/**
 * Unit tests for myModule
 */
define([
    'angular',
    'angular-mocks',
    'path/to/myModule'
], function() {
    'use strict';

    describe('myModule', function() {
        var myModule;

        // Setup before each test
        beforeEach(function() {
            module('domoticz');

            inject(function(_myModule_) {
                myModule = _myModule_;
            });
        });

        // Write your tests
        it('should do something', function() {
            expect(myModule.method()).toBe(true);
        });
    });
});
```

## Mocking Dependencies

```javascript
beforeEach(function() {
    module('domoticz');

    // Mock a dependency
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

## Testing Promises

```javascript
it('should resolve promise', function(done) {
    myService.asyncMethod().then(function(result) {
        expect(result).toBe('success');
        done(); // Signal test completion
    });

    $rootScope.$digest(); // Trigger promise resolution
});
```

## Testing Directives

```javascript
var $compile, $scope, element;

beforeEach(inject(function(_$compile_, _$rootScope_) {
    $compile = _$compile_;
    $scope = _$rootScope_.$new();
}));

it('should compile directive', function() {
    $scope.data = { name: 'test' };
    element = $compile('<my-directive data="data"></my-directive>')($scope);
    $scope.$digest();

    expect(element.html()).toContain('test');
});
```

## Viewing Coverage

After running `npm run test:coverage`, open:
```
s:/Domoticz/www/test/coverage/index.html
```

## Troubleshooting

### Tests won't run
1. Check that Chrome is installed
2. Try: `npm install` again
3. Check for syntax errors in test files

### Module not found
- Verify file path in `define()` array matches actual file location
- Check `test/test-main.js` for correct RequireJS paths

### Tests hang
- Make sure async tests call `done()`
- Ensure promises are resolved with `$rootScope.$digest()`

## Example Test Run

```bash
$ npm test

> domoticz-frontend@1.0.0 test
> karma start test/karma.conf.js

16 02 2026 11:52:14.123:INFO [karma-server]: Karma v6.4.2 server started
16 02 2026 11:52:14.125:INFO [launcher]: Launching browser ChromeHeadless
16 02 2026 11:52:15.456:INFO [Chrome Headless]: Connected

  dashboardService
    ✓ should load favorites
    ✓ should categorize devices
    ... (28 tests)

  deviceDetection
    ✓ should detect mobile
    ✓ should detect tablet
    ... (23 tests)

  dzLightWidget
    ✓ should compile
    ✓ should detect dimmer
    ... (32 tests)

Chrome Headless: Executed 83 of 83 SUCCESS (2.145 secs / 1.987 secs)
```

## Need Help?

- Full documentation: `test/README.md`
- Example tests: `test/unit/dashboard/dashboardService.spec.js`
- Jasmine docs: https://jasmine.github.io/
- Karma docs: https://karma-runner.github.io/
