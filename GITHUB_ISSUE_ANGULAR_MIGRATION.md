# [EPIC] Migrate jQuery-based Pages to Angular Architecture

## Summary

Migrate Lights, Scenes, Utility, and Dashboard pages from jQuery DOM manipulation to Angular-based architecture. This will eliminate timing-related bugs, improve code maintainability, and create a consistent architecture across all Domoticz pages.

## Problem Statement

Currently, 4 major pages (Lights, Scenes, Utility, Dashboard) use jQuery to dynamically generate and manipulate HTML (~9,500 lines of code). This approach has several issues:

### Technical Issues
- ⚠️ **Flash bugs** - Devices flash in filtered views due to race conditions
- ⚠️ **Timing problems** - Requires setTimeout workarounds
- ⚠️ **Hard to test** - DOM manipulation difficult to unit test
- ⚠️ **Performance issues** - Inefficient DOM queries
- ⚠️ **Maintenance burden** - Complex imperative code

### Architectural Issues
- 🔴 **Mixed paradigms** - jQuery and Angular fighting for DOM control
- 🔴 **No component reuse** - Each controller generates HTML strings
- 🔴 **Poor separation** - Business logic mixed with presentation
- 🔴 **Code duplication** - Similar widget rendering in each controller

## Proposed Solution

Convert these pages to use Angular directives and data binding, following the pattern already successfully implemented in Temperature and Weather pages.

### Benefits
- ✅ Eliminate flash bugs (single atomic digest cycle)
- ✅ Remove setTimeout workarounds
- ✅ 60-80% code reduction per controller
- ✅ Better performance (Angular's optimized change detection)
- ✅ Easier testing (component-based unit tests)
- ✅ Consistent architecture across all pages
- ✅ Easier maintenance and onboarding

## Implementation Phases

### Phase 1: Foundation (2 weeks)
Create reusable Angular widget directives:
- [ ] Light/Switch widget directive + template
- [ ] Scene widget directive + template
- [ ] Utility widget directive + template
- [ ] Dashboard card directive + template
- [ ] Unit tests for each widget

### Phase 2: Lights Page (2 weeks)
- [ ] Create new LightsController with Angular architecture
- [ ] Create lights.html template using ng-repeat
- [ ] Implement Angular-native filtering
- [ ] Migrate all device control functions
- [ ] Testing and validation

### Phase 3: Scenes Page (2 weeks)
- [ ] Create new ScenesController
- [ ] Create scenes.html template
- [ ] Implement filtering
- [ ] Scene activation/deactivation functions
- [ ] Testing and validation

### Phase 4: Utility Page (2 weeks)
- [ ] Create new UtilityController
- [ ] Create utility.html template
- [ ] Implement filtering
- [ ] Utility-specific functions
- [ ] Testing and validation

### Phase 5: Dashboard (3 weeks)
- [ ] Create new DashboardController
- [ ] Create dashboard.html template
- [ ] Dashboard card components
- [ ] Mobile vs desktop templates
- [ ] Evohome integration
- [ ] Testing on mobile and desktop

### Phase 6: Cleanup (2 weeks)
- [ ] Remove legacy jQuery controllers (after backup)
- [ ] Remove unused jQuery filter code
- [ ] Clean up unused CSS
- [ ] Remove setTimeout workarounds
- [ ] Performance optimization
- [ ] Documentation updates

**Total Estimated Time:** 13 weeks

## Success Criteria

### Functional
- ✅ All existing functionality works
- ✅ No regressions in device control
- ✅ Filtering works on all pages
- ✅ Real-time updates work correctly
- ✅ Mobile and desktop views work

### Performance
- ✅ Page load time ≤ current implementation
- ✅ Filter response time < 100ms
- ✅ Update handling < 50ms

### Code Quality
- ✅ 60-80% code reduction per controller
- ✅ 80%+ unit test coverage for new code
- ✅ No jQuery DOM manipulation in controllers
- ✅ Passes ESLint with no errors

## Example: Before vs After

### Before (jQuery - ~400 lines per widget)
```javascript
var html = 
    '<div class="item span4 itemBlock" id="' + item.idx + '">\n' +
    '  <section>\n' +
    '    <table>\n' +
    '      <tr>\n' +
    '        <td id="name">' + item.Name + '</td>\n' +
    // ... 50+ lines of string concatenation
    '    </table>\n' +
    '  </section>\n' +
    '</div>';

$('#lightcontent').append(html);

// Later, manual updates
$(id + " #name").html(item.Name);
$(id + " #status").html(status);
// ... many more
```

### After (Angular - ~50 lines total)
```javascript
// Controller
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
```

```html
<!-- Template -->
<dz-light-widget ng-repeat="light in ctrl.lights"
                ng-show="ctrl.matchesFilter(light)"
                item="light">
</dz-light-widget>
```

## Risk Mitigation

### High Risk: Dashboard Complexity (4,093 lines)
**Mitigation:** Break into smaller components, migrate incrementally, use feature flags

### Medium Risk: Device Control Functions
**Mitigation:** Extract into services, unit test thoroughly

### Low Risk: Filtering
**Mitigation:** Already proven in Temperature/Weather pages

## Rollback Plan

Implement each phase behind feature flags:
```javascript
if (window.myglobals.useAngularLights) {
    // New Angular version
} else {
    // Old jQuery version
}
```

This allows:
- Testing without affecting users
- Quick rollback if critical issues found
- Gradual rollout
- A/B testing

## Resources

### Documentation
- [ANGULAR_MIGRATION_PLAN.md](./ANGULAR_MIGRATION_PLAN.md) - Detailed migration guide
- Temperature & Weather controllers - Reference implementations

### Team Requirements
- 2 Senior Developers (Angular + Domoticz knowledge)
- 1 QA Engineer
- 1 Technical Writer

## Related Issues

- Original flash bug: #[insert issue number]
- Angular filtering implementation: PR #[insert PR number]

## Labels

- `enhancement`
- `angular`
- `refactor`
- `frontend`
- `epic`

## CC

@[maintainer1] @[maintainer2] @[maintainer3]

---

**Note:** This is a large refactoring effort but the benefits are significant. The Temperature and Weather pages already prove this approach works well. Starting with a single page (Lights) as a proof of concept is recommended.
