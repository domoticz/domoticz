# [EPIC] Migrate jQuery-based Pages to Angular Architecture

## 📋 Summary

Migrate Lights, Scenes, Utility, and Dashboard pages from jQuery DOM manipulation to Angular-based architecture. This will eliminate timing-related bugs, improve code maintainability, and create a consistent architecture across all Domoticz pages.

**Current State:** 4 major pages use jQuery to dynamically generate HTML (~9,500 lines of code)  
**Goal:** Convert to Angular directives and data binding  
**Timeline:** 13 weeks  
**Expected Impact:** 79% code reduction, eliminate flash bugs, better performance

---

## 🔴 Problem Statement

Currently, 4 major pages (Lights, Scenes, Utility, Dashboard) use jQuery to dynamically generate and manipulate HTML. This approach has significant issues:

### Technical Issues
- ⚠️ **Flash bugs** - Devices flash in filtered views due to race conditions between Angular digest and jQuery DOM updates
- ⚠️ **Timing problems** - Requires setTimeout workarounds to sync Angular and jQuery
- ⚠️ **Hard to test** - DOM manipulation difficult to unit test
- ⚠️ **Performance issues** - Inefficient DOM queries on every update
- ⚠️ **Maintenance burden** - Complex imperative code with string concatenation

### Architectural Issues
- 🔴 **Mixed paradigms** - jQuery and Angular fighting for DOM control
- 🔴 **No component reuse** - Each controller generates HTML strings independently
- 🔴 **Poor separation** - Business logic mixed with presentation code
- 🔴 **Code duplication** - Similar widget rendering logic in multiple controllers
- 🔴 **No type safety** - String concatenation for HTML generation

### Code Statistics
```
Page        Lines    .html() Calls    Complexity
─────────   ─────    ─────────────    ──────────
Lights      2,439    73               High
Scenes      1,146    17               Medium
Utility     1,835    14               Medium
Dashboard   4,093    78               Very High
─────────   ─────    ─────────────    ──────────
TOTAL       9,513    182              
```

---

## ✅ Proposed Solution

Convert these pages to use Angular directives and data binding, following the pattern already successfully implemented in **Temperature** and **Weather** pages.

### Reference Implementations
- ✅ **TemperatureController** - Already using Angular-native filtering
- ✅ **WeatherController** - Already using Angular-native filtering

These pages demonstrate the approach works well and can be used as templates.

### Benefits
- ✅ **Eliminate flash bugs** - Single atomic digest cycle, no race conditions
- ✅ **Remove setTimeout workarounds** - No timing issues
- ✅ **60-80% code reduction** per controller (~7,500 lines removed)
- ✅ **Better performance** - Angular's optimized change detection
- ✅ **Easier testing** - Component-based unit tests
- ✅ **Consistent architecture** - Same patterns across all pages
- ✅ **Easier maintenance** - Declarative templates vs imperative strings
- ✅ **Better onboarding** - Easier for new developers to understand

---

## 📅 Implementation Plan

### Phase 1: Foundation (Weeks 1-2)
**Goal:** Create reusable Angular widget directives

**Tasks:**
- [ ] Create `dzLightWidget` directive + template
- [ ] Create `dzSceneWidget` directive + template
- [ ] Create `dzUtilityWidget` directive + template
- [ ] Create `dzDashboardCard` directive + template (desktop)
- [ ] Create `dzDashboardCardMobile` directive + template (mobile)
- [ ] Create widget controller base class/service
- [ ] Write unit tests for each widget
- [ ] Document component API

**Deliverables:**
- Reusable widget components
- Component documentation
- Unit tests

---

### Phase 2: Lights Page Migration (Weeks 3-4)
**Goal:** Convert Lights page to Angular

**Tasks:**
- [ ] Create new `LightsController` with Angular architecture
- [ ] Implement `ctrl.searchQuery` and `ctrl.matchesFilter()`
- [ ] Create `lights.html` template using `ng-repeat`
- [ ] Use `dzLightWidget` directive
- [ ] Migrate all device control functions (on/off, dimming, color)
- [ ] Implement real-time updates via `device_update` event
- [ ] Test filtering functionality
- [ ] Test device controls (on/off, dim, color picker)
- [ ] Performance testing
- [ ] Deploy with feature flag for A/B testing

**Success Criteria:**
- All existing functionality works
- Filtering works without flash
- Code reduced from ~2,439 to ~500 lines

---

### Phase 3: Scenes Page Migration (Weeks 5-6)
**Goal:** Convert Scenes page to Angular

**Tasks:**
- [ ] Create new `ScenesController` with Angular architecture
- [ ] Implement filtering
- [ ] Create `scenes.html` template using `ng-repeat`
- [ ] Use `dzSceneWidget` directive
- [ ] Migrate scene activation/deactivation functions
- [ ] Test scene controls
- [ ] Test filtering
- [ ] Performance testing
- [ ] Deploy with feature flag

**Success Criteria:**
- All scene functionality works
- Filtering works without flash
- Code reduced from ~1,146 to ~300 lines

---

### Phase 4: Utility Page Migration (Weeks 7-8)
**Goal:** Convert Utility page to Angular

**Tasks:**
- [ ] Create new `UtilityController` with Angular architecture
- [ ] Implement filtering
- [ ] Create `utility.html` template using `ng-repeat`
- [ ] Use `dzUtilityWidget` directive
- [ ] Migrate utility-specific functions
- [ ] Test all utility device types
- [ ] Test filtering
- [ ] Performance testing
- [ ] Deploy with feature flag

**Success Criteria:**
- All utility devices display correctly
- Filtering works without flash
- Code reduced from ~1,835 to ~400 lines

---

### Phase 5: Dashboard Migration with Mobile/Desktop Separation (Weeks 9-11)
**Goal:** Convert Dashboard to Angular with separate mobile and desktop implementations

**Note:** Dashboard is the most complex (4,093 lines) and currently mixes mobile/desktop logic throughout.

#### Current Dashboard Issues
- Uses `MobilePhoneDetection()` to set body ID
- Throughout code: `if (DashboardType == 2 || window.myglobals.ismobile)`
- Different HTML generation for mobile vs desktop
- Mixed rendering logic makes code hard to maintain

#### Strategy: Separate Controllers and Templates

**5.1 Create Shared Dashboard Service**
- [ ] Create `dashboardService.js` for common functionality
- [ ] Implement `loadFavorites()` method
- [ ] Implement device control methods
- [ ] Migrate Evohome integration to service
- [ ] Write service unit tests

**5.2 Create Desktop Dashboard**
- [ ] Create `DashboardDesktopController.js`
- [ ] Create `dashboard_desktop.html` template
- [ ] Use grid/card layout optimized for desktop
- [ ] Implement search filtering
- [ ] Support drag-and-drop ordering
- [ ] Test on various desktop browsers

**5.3 Create Mobile Dashboard**
- [ ] Create `DashboardMobileController.js`
- [ ] Create `dashboard_mobile.html` template
- [ ] Use list layout optimized for mobile
- [ ] Simplified interface for touch
- [ ] Test on mobile devices (iOS, Android)
- [ ] Test on tablets

**5.4 Implement Smart Routing**
- [ ] Add device detection in routing
- [ ] Route to correct template based on device
- [ ] Handle manual override (if needed)
- [ ] Test switching between devices

**5.5 Create Dashboard Components**
- [ ] Desktop dashboard card component
- [ ] Mobile dashboard card component
- [ ] Shared component logic
- [ ] Unit tests for components

**Benefits of Separation:**
- ✅ Cleaner code - no mixed mobile/desktop logic
- ✅ Better optimization - each view optimized separately
- ✅ Better performance - load only what's needed
- ✅ Easier testing - test independently
- ✅ Simpler maintenance - clear separation

**Success Criteria:**
- All dashboard functionality works on desktop
- All dashboard functionality works on mobile
- Filtering works (desktop)
- Code reduced from ~4,093 to ~800 lines total
- No mobile/desktop conditional logic in controllers

---

### Phase 6: Cleanup and Optimization (Weeks 12-13)
**Goal:** Remove legacy code and optimize

**6.1 Remove Legacy Code**
- [ ] Backup old jQuery-based controllers
- [ ] Delete old jQuery controllers
- [ ] Remove unused jQuery filter code from `domoticz.js`
- [ ] Clean up unused CSS classes
- [ ] Remove all setTimeout workarounds
- [ ] Update routing to remove feature flags

**6.2 Optimize and Refactor**
- [ ] Extract common widget logic into services
- [ ] Create shared filter service
- [ ] Optimize templates (use one-time binding where appropriate)
- [ ] Add JSDoc comments for all public APIs
- [ ] Update developer documentation
- [ ] Performance profiling and optimization

**6.3 Comprehensive Testing**
- [ ] Integration testing across all pages
- [ ] Performance benchmarking (compare before/after)
- [ ] Browser compatibility testing (Chrome, Firefox, Safari, Edge)
- [ ] Mobile testing (iOS Safari, Chrome Android)
- [ ] Tablet testing
- [ ] Accessibility audit (WCAG compliance)
- [ ] Regression testing

**6.4 Documentation**
- [ ] Update user documentation
- [ ] Update developer documentation
- [ ] Create migration guide for contributors
- [ ] Document component API
- [ ] Update README

---

## 🎯 Success Criteria

### Functional Requirements
- ✅ All existing functionality works on all pages
- ✅ No regressions in device control
- ✅ Filtering works on all pages without flash
- ✅ Real-time updates work correctly
- ✅ Drag-and-drop ordering works (where applicable)
- ✅ Mobile and desktop views work correctly
- ✅ All device types display correctly

### Performance Requirements
- ✅ Page load time ≤ current implementation
- ✅ Filter response time < 100ms
- ✅ Device update handling < 50ms
- ✅ Memory usage ≤ current implementation
- ✅ Smooth scrolling and interactions

### Code Quality Requirements
- ✅ 60-80% code reduction per controller
- ✅ 80%+ unit test coverage for new code
- ✅ JSDoc comments for all public APIs
- ✅ No jQuery DOM manipulation in controllers
- ✅ Passes ESLint with no errors
- ✅ No console warnings or errors

### User Experience Requirements
- ✅ No visible behavior changes (except bug fixes)
- ✅ Smooth animations and transitions
- ✅ Responsive on all screen sizes
- ✅ Touch-friendly on mobile
- ✅ Accessible (keyboard navigation, screen readers)

---

## 📊 Expected Impact

### Code Reduction
```
Page        Before      After       Reduction
──────────  ──────────  ─────────   ─────────
Lights      2,439       ~500        80%
Scenes      1,146       ~300        74%
Utility     1,835       ~400        78%
Dashboard   4,093       ~800        80%
──────────  ──────────  ─────────   ─────────
TOTAL       9,513       ~2,000      79%
```

### Bug Elimination
- **Flash bugs:** Eliminated (atomic digest cycle)
- **setTimeout workarounds:** Removed (4 → 0)
- **Manual DOM updates:** Removed (500+ → 0)
- **jQuery .html() calls:** Removed (182 → 0)

### Performance Improvements (Estimated)
- **Page load:** 20-30% faster
- **Filter response:** 50% faster
- **Memory usage:** 20% reduction
- **Rendering:** Smoother, no jank

---

## 🛡️ Risk Assessment and Mitigation

### High Risk Items

**1. Dashboard Complexity (4,093 lines)**
- **Risk:** Most complex page, highest chance of issues
- **Mitigation:** 
  - Break into smaller components
  - Separate mobile/desktop implementations
  - Incremental migration with feature flags
  - Extensive testing on multiple devices

**2. Evohome Integration**
- **Risk:** Complex state management, easy to break
- **Mitigation:**
  - Create dedicated Evohome service
  - Thorough unit testing
  - Test with actual Evohome hardware

**3. Mobile vs Desktop Rendering**
- **Risk:** Different devices need different UIs
- **Mitigation:**
  - Separate controllers and templates
  - Clear device detection strategy
  - Test on wide range of devices
  - Support manual override if needed

### Medium Risk Items

**1. Device Control Functions**
- **Risk:** Many different device types with different controls
- **Mitigation:**
  - Extract into services
  - Unit test each device type
  - Regression testing

**2. Real-time Updates (WebSocket)**
- **Risk:** Events might not wire up correctly
- **Mitigation:**
  - Ensure event listeners work correctly
  - Test with actual MQTT/WebSocket updates
  - Monitor for memory leaks

**3. Drag and Drop Ordering**
- **Risk:** Complex interaction, might break
- **Mitigation:**
  - Use Angular drag-drop library
  - Thorough testing
  - Fallback to manual ordering

### Low Risk Items

**1. Filtering** - Already proven in Temperature/Weather  
**2. Templates** - Straightforward conversion  
**3. CSS** - Mostly unchanged

---

## 🔄 Rollback Plan

Each phase will be implemented behind **feature flags** to allow safe rollback:

```javascript
// In app config
app.config(function($routeProvider) {
    if (window.myglobals.useAngularLights) {
        // New Angular version
        $routeProvider.when('/Lights', {
            templateUrl: 'views/lights_angular.html',
            controller: 'LightsControllerAngular'
        });
    } else {
        // Old jQuery version (fallback)
        $routeProvider.when('/Lights', {
            templateUrl: 'views/lights_jquery.html',
            controller: 'LightsControllerJQuery'
        });
    }
});
```

This allows:
- ✅ Testing new version without affecting users
- ✅ Quick rollback if critical issues found
- ✅ Gradual rollout to users (10%, 25%, 50%, 100%)
- ✅ A/B testing if needed
- ✅ Per-page rollback (can rollback one page without affecting others)

---

## 💡 Code Examples

### Before (jQuery - 400+ lines)
```javascript
// String concatenation for HTML generation
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

// Manual filtering with timing issues
var query = $('.jsLiveSearch').val();
if (query.length > 0) {
    $('.itemBlock').each(function() {
        if (matches) {
            $(this).show();
        } else {
            $(this).hide();
        }
    });
}

// Requires setTimeout workaround
setTimeout(function() {
    RefreshLiveSearch();
}, 0);
```

**Issues:**
- ❌ 400+ lines per widget type
- ❌ Error-prone string concatenation
- ❌ Manual DOM manipulation
- ❌ Race conditions require setTimeout
- ❌ Hard to test
- ❌ Hard to maintain

### After (Angular - 50 lines total)
```javascript
// Controller (25 lines)
app.controller('LightsController', function($scope, livesocket) {
    var ctrl = this;
    
    ctrl.lights = [];
    ctrl.searchQuery = '';
    
    ctrl.matchesFilter = function(item) {
        if (!ctrl.searchQuery) return true;
        return item.searchText.toLowerCase()
            .indexOf(ctrl.searchQuery.toLowerCase()) !== -1;
    };
    
    ctrl.getFilteredCount = function() {
        return ctrl.lights.filter(ctrl.matchesFilter).length;
    };
    
    $scope.$on('device_update', function(event, device) {
        ctrl.lights.forEach(function(item, index) {
            if (item.idx === device.idx) {
                ctrl.lights[index] = device; // That's it!
            }
        });
    });
    
    init();
});
```

```html
<!-- Template (25 lines) -->
<div class="devicesList">
    <input ng-model="ctrl.searchQuery" placeholder="Search devices...">
    <div>Showing {{ctrl.getFilteredCount()}} devices</div>
    
    <div class="row divider">
        <dz-light-widget ng-repeat="light in ctrl.lights"
                        ng-show="ctrl.matchesFilter(light)"
                        class="span4 itemBlock"
                        item="light">
        </dz-light-widget>
    </div>
</div>
```

**Benefits:**
- ✅ 50 lines total (was 400+) - 87% reduction
- ✅ Declarative template
- ✅ Automatic updates via data binding
- ✅ No race conditions
- ✅ Easy to test
- ✅ Easy to maintain

---

## 👥 Resource Requirements

### Development Team
- **2 Senior Developers** (familiar with Angular and Domoticz architecture)
- **1 QA Engineer** (testing and validation)
- **1 Technical Writer** (documentation)

### Time Allocation
- **Total:** 13 weeks
- **Per Page Average:** 2-3 weeks
- **Dashboard:** 3 weeks (most complex)
- **Cleanup:** 2 weeks

### Infrastructure
- Development environment
- Staging environment for testing
- Automated testing pipeline
- Code review process
- Feature flag system

---

## 📚 Documentation

Comprehensive migration documentation has been created:

1. **[ANGULAR_MIGRATION_PLAN.md](./ANGULAR_MIGRATION_PLAN.md)** (18,660 chars)
   - Executive summary
   - Detailed phase-by-phase plan
   - Coding standards and templates
   - Risk assessment
   - Success criteria

2. **[ANGULAR_MIGRATION_QUICK_REFERENCE.md](./ANGULAR_MIGRATION_QUICK_REFERENCE.md)** (12,683 chars)
   - Architecture comparison
   - Code templates
   - Common patterns
   - Pitfalls to avoid
   - Performance tips

---

## 🚀 Next Steps

1. **Review and Approval** ✋
   - Present plan to development team
   - Get stakeholder approval
   - Allocate resources

2. **Setup** 🔧
   - Create development environment
   - Setup staging environment
   - Setup feature flag system
   - Create project board

3. **Start Phase 1** 🎯
   - Create widget directives
   - 2-week sprint
   - Weekly check-ins

4. **Iterate** 🔄
   - Complete each phase
   - Test thoroughly
   - Get feedback
   - Adjust as needed

---

## 🏷️ Labels

- `enhancement`
- `angular`
- `refactor`
- `frontend`
- `epic`
- `good-first-issue` (for smaller sub-tasks)
- `help-wanted`

---

## 📎 Related

- Original flash bug issue: (link to be added)
- Angular filtering PR: (link to be added)
- Temperature/Weather reference implementation

---

## 💬 Discussion

This is a significant refactoring effort, but the benefits are substantial:

✅ **Proven approach** - Temperature and Weather pages already work well  
✅ **Low risk** - Phased approach with feature flags  
✅ **High value** - 79% code reduction, eliminate bugs, better performance  
✅ **Future-proof** - Modern architecture, easier to maintain

**Recommendation:** Start with Phase 1 (widget creation) and Phase 2 (Lights page) as a proof of concept. Success there will validate the approach for the remaining pages.

---

**CC:** @maintainers - Please review and provide feedback
