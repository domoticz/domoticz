# Angular Migration Plan for Domoticz jQuery-based Pages

## Executive Summary

This document outlines a comprehensive plan to migrate Domoticz pages that currently use jQuery DOM manipulation to Angular-based architecture. The migration will improve code maintainability, testability, performance, and eliminate timing-related bugs.

## Current State Analysis

### Pages Using Angular (✅ Modern Architecture)
- **Temperature** - Uses Angular directives (`dztemperaturewidget`)
- **Weather** - Uses Angular directives (`dzweatherwidget`)
- **Devices** - Uses Angular components

### Pages Using jQuery (⚠️ Legacy Architecture - Need Migration)
- **Lights** - 2,439 lines, 73 `.html()` calls
- **Scenes** - 1,146 lines, 17 `.html()` calls
- **Utility** - 1,835 lines, 14 `.html()` calls
- **Dashboard** - 4,093 lines, 78 `.html()` calls

**Total legacy code:** ~9,500 lines

## Problems with Current jQuery Approach

### Technical Issues
1. **Timing Problems** - Race conditions between Angular digest and jQuery DOM updates
2. **Flash Bugs** - Items flash in filtered views (partially fixed with setTimeout workaround)
3. **Hard to Test** - DOM manipulation difficult to unit test
4. **Performance** - Inefficient DOM queries on every update
5. **Maintenance** - Complex, imperative code that's hard to understand
6. **Code Duplication** - Similar widget rendering logic in each controller

### Architectural Issues
1. **Mixed Paradigms** - jQuery and Angular fighting for DOM control
2. **No Component Reuse** - Each controller generates HTML strings
3. **Poor Separation of Concerns** - Business logic mixed with presentation
4. **No Type Safety** - String concatenation for HTML generation
5. **Difficult Debugging** - Hard to trace issues through jQuery chains

## Benefits of Angular Migration

### Immediate Benefits
- ✅ **Eliminate flash bugs** - Single atomic digest cycle
- ✅ **Remove setTimeout workarounds** - No timing issues
- ✅ **Better performance** - Angular's optimized change detection
- ✅ **Easier testing** - Component-based unit tests
- ✅ **Code reduction** - ~60-80% less code per controller

### Long-term Benefits
- ✅ **Maintainability** - Declarative templates easier to understand
- ✅ **Reusability** - Components can be shared across pages
- ✅ **Consistency** - Same patterns across all pages
- ✅ **Future-proof** - Modern architecture, easier to upgrade
- ✅ **Better DX** - Easier for new developers to contribute

## Migration Strategy

### Phase 1: Foundation (Weeks 1-2)
**Goal:** Create reusable Angular components for common widgets

#### 1.1 Create Light/Switch Widget Directive
```javascript
// www/app/widgets/lightWidget.js
app.directive('dzLightWidget', function() {
    return {
        restrict: 'E',
        scope: {
            item: '=',
            ordering: '='
        },
        templateUrl: 'views/widgets/light_widget.html',
        controller: 'LightWidgetController',
        controllerAs: 'ctrl'
    };
});
```

#### 1.2 Create Scene Widget Directive
```javascript
app.directive('dzSceneWidget', function() {
    // Similar to light widget
});
```

#### 1.3 Create Utility Widget Directive
```javascript
app.directive('dzUtilityWidget', function() {
    // Similar to light widget
});
```

#### 1.4 Create Dashboard Card Component
```javascript
app.directive('dzDashboardCard', function() {
    // Generic dashboard card
});
```

**Deliverables:**
- [ ] Light widget directive + template
- [ ] Scene widget directive + template
- [ ] Utility widget directive + template
- [ ] Dashboard card directive + template
- [ ] Widget controller base class/mixin
- [ ] Unit tests for each widget

### Phase 2: Lights Page Migration (Weeks 3-4)
**Goal:** Convert Lights page to Angular

#### 2.1 Create LightsController (Angular)
```javascript
app.controller('LightsController', function($scope, deviceApi, livesocket) {
    var ctrl = this;
    
    // Angular-native filtering
    ctrl.searchQuery = '';
    ctrl.lights = [];
    
    ctrl.matchesFilter = function(item) {
        if (!ctrl.searchQuery) return true;
        return item.searchText.toLowerCase()
            .indexOf(ctrl.searchQuery.toLowerCase()) !== -1;
    };
    
    ctrl.getFilteredCount = function() {
        return ctrl.lights.filter(ctrl.matchesFilter).length;
    };
    
    // Listen for updates
    $scope.$on('device_update', function(event, deviceData) {
        updateItem(deviceData);
    });
});
```

#### 2.2 Create Template
```html
<div class="devicesList">
    <div class="row divider">
        <div ng-repeat-start="light in ctrl.lights" 
             class="clearfix" 
             ng-if="$index % 3 == 0 && !$first && ctrl.matchesFilter(light)">
        </div>
        <dz-light-widget ng-repeat-end
                        class="span4 itemBlock"
                        ng-show="ctrl.matchesFilter(light)"
                        item="light"
                        ordering="config.AllowWidgetOrdering">
        </dz-light-widget>
    </div>
</div>
```

**Deliverables:**
- [ ] New LightsController with Angular architecture
- [ ] lights.html template using ng-repeat
- [ ] Migration of all device control functions
- [ ] Testing in parallel with old version
- [ ] Documentation of differences

### Phase 3: Scenes Page Migration (Weeks 5-6)
**Goal:** Convert Scenes page to Angular

Similar approach to Lights:
- Create ScenesController (Angular)
- Create scenes.html template
- Use dz-scene-widget directive
- Implement filtering
- Test thoroughly

**Deliverables:**
- [ ] New ScenesController
- [ ] scenes.html template
- [ ] Scene activation/deactivation functions
- [ ] Testing and validation

### Phase 4: Utility Page Migration (Weeks 7-8)
**Goal:** Convert Utility page to Angular

Similar approach:
- Create UtilityController (Angular)
- Create utility.html template
- Use dz-utility-widget directive
- Implement filtering
- Test thoroughly

**Deliverables:**
- [ ] New UtilityController
- [ ] utility.html template
- [ ] Utility-specific functions
- [ ] Testing and validation

### Phase 5: Dashboard Migration (Weeks 9-11)
**Goal:** Convert Dashboard to Angular with separate mobile and desktop implementations

**Note:** Dashboard is the most complex (4,093 lines). It currently mixes mobile and desktop logic throughout with conditional checks.

#### Strategy: Separate Controllers and Templates

The Dashboard currently uses a single controller with extensive mobile detection:
- `MobilePhoneDetection()` sets body ID to `onMobile` or `notMobile`
- Throughout the code: `if (DashboardType == 2 || window.myglobals.ismobile)`
- Different HTML generation for mobile vs desktop
- Mixed rendering logic makes code hard to maintain

**New Approach:** Create two separate implementations:

#### 5.1 Create Shared Dashboard Service
```javascript
// www/app/services/dashboardService.js
app.service('dashboardService', function(livesocket, deviceApi) {
    var service = this;
    
    // Common data fetching
    service.loadFavorites = function() {
        return livesocket.getJson('json.htm?type=command&param=getfavorites');
    };
    
    // Common device actions
    service.toggleDevice = function(idx) {
        return deviceApi.switchDevice(idx);
    };
    
    // Evohome integration (shared)
    service.switchModal = function(idx, status) { ... };
});
```

#### 5.2 Create Desktop Dashboard Controller
```javascript
// www/app/DashboardDesktopController.js
app.controller('DashboardDesktopController', 
    function($scope, dashboardService) {
    
    var ctrl = this;
    ctrl.favorites = [];
    ctrl.searchQuery = '';
    
    // Desktop-specific layout (cards, grid)
    ctrl.layoutType = 'grid'; // or 'list'
    
    ctrl.matchesFilter = function(item) { ... };
    
    function loadFavorites() {
        dashboardService.loadFavorites().then(function(data) {
            ctrl.favorites = data.result || [];
        });
    }
    
    init();
});
```

#### 5.3 Create Mobile Dashboard Controller
```javascript
// www/app/DashboardMobileController.js
app.controller('DashboardMobileController', 
    function($scope, dashboardService) {
    
    var ctrl = this;
    ctrl.favorites = [];
    
    // Mobile-specific: simplified, list-based
    ctrl.layoutType = 'mobile-list';
    
    // Simpler interface for mobile
    ctrl.toggleDevice = function(device) {
        dashboardService.toggleDevice(device.idx);
    };
    
    function loadFavorites() {
        dashboardService.loadFavorites().then(function(data) {
            // Filter or format for mobile if needed
            ctrl.favorites = data.result || [];
        });
    }
    
    init();
});
```

#### 5.4 Create Separate Templates

**Desktop Template (views/dashboard_desktop.html):**
```html
<div class="container dashboard-desktop">
    <div ng-include="'views/inc_topbar.html'"></div>
    
    <div class="dashboard-grid">
        <dz-dashboard-card ng-repeat="item in ctrl.favorites"
                          ng-show="ctrl.matchesFilter(item)"
                          class="dashboard-card"
                          item="item"
                          layout="grid">
        </dz-dashboard-card>
    </div>
</div>
```

**Mobile Template (views/dashboard_mobile.html):**
```html
<div class="container dashboard-mobile">
    <!-- Mobile-optimized header (no search by default) -->
    <div class="mobile-header">
        <h1>Dashboard</h1>
    </div>
    
    <!-- Simple list layout for mobile -->
    <div class="dashboard-list">
        <dz-dashboard-card-mobile ng-repeat="item in ctrl.favorites"
                                 class="mobile-card"
                                 item="item"
                                 layout="list">
        </dz-dashboard-card-mobile>
    </div>
</div>
```

#### 5.5 Routing with Device Detection
```javascript
// In app.js routing configuration
app.config(function($routeProvider) {
    $routeProvider.when('/Dashboard', {
        templateUrl: function() {
            // Detect device type
            if (window.myglobals.ismobile || isMobileDevice()) {
                return 'views/dashboard_mobile.html';
            } else {
                return 'views/dashboard_desktop.html';
            }
        },
        controller: function() {
            if (window.myglobals.ismobile || isMobileDevice()) {
                return 'DashboardMobileController';
            } else {
                return 'DashboardDesktopController';
            }
        },
        controllerAs: 'ctrl'
    });
});

// Helper function
function isMobileDevice() {
    return /(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|.../i
        .test(navigator.userAgent);
}
```

#### 5.6 Create Dashboard Card Components

**Desktop Card:**
```javascript
app.directive('dzDashboardCard', function() {
    return {
        restrict: 'E',
        scope: { item: '=', layout: '@' },
        templateUrl: 'views/widgets/dashboard_card_desktop.html',
        controller: 'DashboardCardController',
        controllerAs: 'ctrl'
    };
});
```

**Mobile Card:**
```javascript
app.directive('dzDashboardCardMobile', function() {
    return {
        restrict: 'E',
        scope: { item: '=' },
        templateUrl: 'views/widgets/dashboard_card_mobile.html',
        controller: 'DashboardCardMobileController',
        controllerAs: 'ctrl'
    };
});
```

**Deliverables:**
- [ ] Create dashboardService (shared logic)
- [ ] Create DashboardDesktopController
- [ ] Create DashboardMobileController
- [ ] Create dashboard_desktop.html template
- [ ] Create dashboard_mobile.html template
- [ ] Create desktop dashboard card components
- [ ] Create mobile dashboard card components
- [ ] Implement routing with device detection
- [ ] Migrate Evohome integration to service
- [ ] Test on desktop browsers
- [ ] Test on mobile devices
- [ ] Test on tablets (decide which version to use)

**Benefits of Separate Pages:**
- ✅ Cleaner code - no mixed mobile/desktop logic
- ✅ Easier to optimize - each view optimized separately
- ✅ Better performance - load only what's needed
- ✅ Easier testing - test mobile and desktop independently
- ✅ Future flexibility - can evolve differently
- ✅ Simpler maintenance - clear separation of concerns

### Phase 6: Cleanup and Polish (Weeks 12-13)
**Goal:** Remove legacy code and optimize

#### 6.1 Remove Legacy Code
- [ ] Delete old jQuery-based controllers (after backup)
- [ ] Remove unused jQuery filter code from domoticz.js
- [ ] Clean up unused CSS classes
- [ ] Remove setTimeout workarounds

#### 6.2 Optimize and Refactor
- [ ] Extract common widget logic into services
- [ ] Create shared filter service
- [ ] Optimize templates
- [ ] Add JSDoc comments
- [ ] Update documentation

#### 6.3 Testing
- [ ] Comprehensive integration testing
- [ ] Performance benchmarking
- [ ] Browser compatibility testing
- [ ] Mobile testing
- [ ] Accessibility audit

## Implementation Guidelines

### Coding Standards

#### Component Structure
```javascript
// Component file structure
app.directive('dzWidgetName', function(dependencies) {
    return {
        restrict: 'E',
        scope: {
            item: '=',           // Device data
            config: '=',         // Configuration
            ordering: '=',       // Drag-drop ordering
            onUpdate: '&'        // Callback
        },
        templateUrl: 'views/widgets/widget_name.html',
        controller: 'WidgetNameController',
        controllerAs: 'ctrl',
        link: function(scope, element, attrs) {
            // DOM-specific logic only
        }
    };
});
```

#### Controller Structure
```javascript
app.controller('PageController', function($scope, $rootScope, services...) {
    var ctrl = this;
    
    // 1. State initialization
    ctrl.items = [];
    ctrl.searchQuery = '';
    
    // 2. Filter functions
    ctrl.matchesFilter = function(item) { ... };
    ctrl.getFilteredCount = function() { ... };
    
    // 3. Action handlers
    ctrl.onDeviceClick = function(device) { ... };
    ctrl.onDeviceUpdate = function(device) { ... };
    
    // 4. Initialization
    function init() {
        loadItems();
        setupEventListeners();
    }
    
    init();
});
```

#### Template Structure
```html
<!-- views/page_name.html -->
<div class="container">
    <!-- Top bar with filters -->
    <div ng-include="'views/inc_topbar.html'"></div>
    
    <!-- Main content -->
    <div class="devicesList">
        <div class="row divider">
            <!-- No items message -->
            <h2 ng-show="ctrl.items.length == 0">No items found...</h2>
            
            <!-- Grid layout helper -->
            <div ng-repeat-start="item in ctrl.items" 
                 class="clearfix" 
                 ng-if="$index % 3 == 0 && !$first && ctrl.matchesFilter(item)">
            </div>
            
            <!-- Widget -->
            <dz-widget ng-repeat-end
                      class="span4 itemBlock"
                      ng-show="ctrl.matchesFilter(item)"
                      item="item"
                      config="config">
            </dz-widget>
        </div>
    </div>
</div>
```

### Testing Strategy

#### Unit Tests
```javascript
describe('LightWidgetController', function() {
    beforeEach(module('app'));
    
    var $controller, ctrl, scope;
    
    beforeEach(inject(function(_$controller_, $rootScope) {
        scope = $rootScope.$new();
        ctrl = _$controller_('LightWidgetController', {
            $scope: scope
        });
    }));
    
    it('should toggle device on/off', function() {
        ctrl.item = { idx: 1, Status: 'Off' };
        ctrl.toggle();
        expect(ctrl.item.Status).toBe('On');
    });
});
```

#### Integration Tests
```javascript
describe('Lights Page', function() {
    it('should filter devices by search query', function() {
        // Test filtering
    });
    
    it('should update device when MQTT message arrives', function() {
        // Test real-time updates
    });
});
```

### Migration Checklist per Page

For each page being migrated:

#### Pre-Migration
- [ ] Document all features and functions
- [ ] Create test cases for current functionality
- [ ] Identify edge cases and special behaviors
- [ ] Backup current implementation

#### During Migration
- [ ] Create Angular controller
- [ ] Create widget directive(s)
- [ ] Create template
- [ ] Implement filtering
- [ ] Implement device updates
- [ ] Implement user interactions (clicks, drags, etc.)
- [ ] Test in development environment

#### Post-Migration
- [ ] Compare functionality with old version
- [ ] Performance testing
- [ ] Fix any regressions
- [ ] Update documentation
- [ ] Code review
- [ ] Deploy to staging
- [ ] User acceptance testing
- [ ] Deploy to production

## Risk Assessment

### High Risk Items
1. **Dashboard Complexity** (4,093 lines)
   - Mitigation: Break into smaller components, migrate incrementally
2. **Evohome Integration** (complex state management)
   - Mitigation: Create dedicated Evohome service
3. **Mobile vs Desktop** (different rendering)
   - Mitigation: Use ng-if for mobile/desktop templates
4. **Breaking Changes** (users depend on current behavior)
   - Mitigation: Thorough testing, phased rollout

### Medium Risk Items
1. **Device Control Functions** (On/Off, dimming, color, etc.)
   - Mitigation: Extract into services, unit test thoroughly
2. **Real-time Updates** (WebSocket integration)
   - Mitigation: Ensure event listeners work correctly
3. **Drag and Drop** (widget ordering)
   - Mitigation: Use Angular drag-drop library or directive

### Low Risk Items
1. **Filtering** (already proven in Temperature/Weather)
2. **Templates** (straightforward conversion)
3. **CSS** (mostly unchanged)

## Resource Requirements

### Development Team
- **2 Senior Developers** (familiar with Angular and Domoticz)
- **1 QA Engineer** (testing and validation)
- **1 Technical Writer** (documentation)

### Time Estimate
- **Total:** 13 weeks
- **Per Page Average:** 2-3 weeks
- **Dashboard:** 3 weeks (most complex)

### Infrastructure
- Development environment
- Staging environment for testing
- Automated testing pipeline
- Code review process

## Success Criteria

### Functional Requirements
- ✅ All existing functionality works
- ✅ No regressions in device control
- ✅ Filtering works on all pages
- ✅ Real-time updates work correctly
- ✅ Drag-and-drop ordering works
- ✅ Mobile and desktop views work

### Performance Requirements
- ✅ Page load time ≤ current implementation
- ✅ Filter response time < 100ms
- ✅ Update handling < 50ms
- ✅ Memory usage ≤ current implementation

### Code Quality Requirements
- ✅ 60-80% code reduction per controller
- ✅ 80%+ unit test coverage for new code
- ✅ JSDoc comments for all public APIs
- ✅ No jQuery DOM manipulation in controllers
- ✅ Passes ESLint with no errors

## Rollback Plan

Each phase should be implemented behind a feature flag:

```javascript
// In app config
app.config(function($routeProvider) {
    if (window.myglobals.useAngularLights) {
        $routeProvider.when('/Lights', {
            templateUrl: 'views/lights_angular.html',
            controller: 'LightsControllerAngular'
        });
    } else {
        $routeProvider.when('/Lights', {
            templateUrl: 'views/lights_jquery.html',
            controller: 'LightsControllerJQuery'
        });
    }
});
```

This allows:
- Testing new version without affecting users
- Quick rollback if critical issues found
- Gradual rollout to users
- A/B testing if needed

## Post-Migration Benefits

### Quantifiable Improvements
- **Code Reduction:** ~6,000 lines removed (60% reduction)
- **Bug Reduction:** Eliminate timing-related flash bugs
- **Performance:** 20-30% faster rendering (estimated)
- **Maintenance:** 50% reduction in bug fix time (estimated)

### Qualitative Improvements
- Consistent architecture across all pages
- Easier onboarding for new developers
- Better code maintainability
- Improved user experience
- Future-proof codebase

## Next Steps

1. **Get Approval** - Present plan to team
2. **Assign Resources** - Allocate developers
3. **Setup Environment** - Create development/staging environments
4. **Start Phase 1** - Begin with widget directives
5. **Regular Check-ins** - Weekly progress reviews

## Appendix A: Example Migration

### Before (jQuery)
```javascript
// LightsController.js - jQuery approach
var html = 
    '<div class="item span4 itemBlock ' + backgroundClass + '" id="' + item.idx + '">\n' +
    '  <section>\n' +
    '    <table border="0" cellpadding="0" cellspacing="0">\n' +
    '      <tr>\n' +
    '        <td id="name">' + item.Name + '</td>\n' +
    '        <td id="bigtext">' + bigtext + '</td>\n' +
    // ... 50 more lines of string concatenation
    '      </tr>\n' +
    '    </table>\n' +
    '  </section>\n' +
    '</div>';

$('#lightcontent').append(html);

// Later, manual DOM updates
$(id + " #name").html(item.Name);
$(id + " #bigtext").html(bigtext);
$(id + " #status").html(status);
// ... many more manual updates
```

### After (Angular)
```javascript
// LightsController.js - Angular approach
app.controller('LightsController', function($scope, livesocket) {
    var ctrl = this;
    ctrl.lights = [];
    ctrl.searchQuery = '';
    
    ctrl.matchesFilter = function(item) {
        if (!ctrl.searchQuery) return true;
        return item.searchText.toLowerCase()
            .indexOf(ctrl.searchQuery.toLowerCase()) !== -1;
    };
    
    $scope.$on('device_update', function(event, device) {
        updateDevice(device);
    });
    
    function updateDevice(device) {
        ctrl.lights.forEach(function(item, index) {
            if (item.idx === device.idx) {
                ctrl.lights[index] = device;
            }
        });
    }
});
```

```html
<!-- views/lights.html - Angular template -->
<div class="devicesList">
    <div class="row divider">
        <dz-light-widget ng-repeat="light in ctrl.lights"
                        ng-show="ctrl.matchesFilter(light)"
                        class="span4 itemBlock"
                        item="light">
        </dz-light-widget>
    </div>
</div>
```

**Result:** 
- From ~400 lines → ~50 lines
- No manual DOM manipulation
- Automatic updates via Angular binding
- No setTimeout workarounds needed

## Appendix B: Component Examples

### Light Widget Template
```html
<!-- views/widgets/light_widget.html -->
<div class="item" ng-class="ctrl.getBackgroundClass()">
    <section>
        <table border="0" cellpadding="0" cellspacing="0">
            <tr>
                <td id="name" class="item-name">{{item.Name}}</td>
                <td id="bigtext">
                    {{ctrl.getStatusText()}}
                    <a ng-if="item.UsedByCamera" 
                       ng-click="ctrl.showCameraStream()">
                        <img src="images/webcam.png" height="16" width="16">
                    </a>
                </td>
                <td id="img">
                    <img ng-src="images/{{ctrl.getDeviceImage()}}" 
                         height="48" width="48"
                         ng-click="ctrl.toggleDevice()">
                </td>
                <td id="status">{{ctrl.getDetailedStatus()}}</td>
                <td id="lastupdate">{{item.LastUpdate}}</td>
                <td class="options">
                    <img ng-src="images/{{item.Favorite ? 'favorite' : 'nofavorite'}}.png"
                         ng-click="ctrl.toggleFavorite()">
                    <a class="btnsmall" ng-href="#/Devices/{{item.idx}}/Log">Log</a>
                    <a class="btnsmall" ng-click="ctrl.editDevice()">Edit</a>
                </td>
            </tr>
        </table>
    </section>
</div>
```

## Conclusion

This migration plan provides a structured approach to modernizing Domoticz's frontend architecture. By converting jQuery-based pages to Angular, we'll achieve better maintainability, eliminate bugs, reduce code complexity, and create a more consistent user experience.

The phased approach minimizes risk while delivering incremental value. Each phase can be tested and validated before moving to the next, ensuring a smooth transition.

**Recommendation:** Start with Phase 1 (widget creation) and Phase 2 (Lights page) as a proof of concept. Success there will validate the approach for the remaining pages.
