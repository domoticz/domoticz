# Angular Migration - Getting Started

This directory contains tools and examples to help with the Angular migration project.

## Quick Links

- **[NEW_GITHUB_ISSUE.md](./NEW_GITHUB_ISSUE.md)** - Complete GitHub issue (ready to post)
- **[ANGULAR_MIGRATION_PLAN.md](./ANGULAR_MIGRATION_PLAN.md)** - Detailed 13-week plan
- **[ANGULAR_MIGRATION_QUICK_REFERENCE.md](./ANGULAR_MIGRATION_QUICK_REFERENCE.md)** - Developer guide

## Three Steps to Get Started

### Step 1: Create the GitHub Issue

```bash
./create_github_issue.sh
```

This will show you instructions for creating the GitHub issue. The content is in `NEW_GITHUB_ISSUE.md`.

**Manual steps:**
1. Go to https://github.com/domoticz/domoticz/issues/new
2. Copy content from `NEW_GITHUB_ISSUE.md`
3. Set title: `[EPIC] Migrate jQuery-based Pages to Angular Architecture`
4. Add labels: `enhancement`, `angular`, `refactor`, `frontend`, `epic`
5. Submit!

### Step 2: Run Migration Analysis

Analyze the current codebase to understand migration complexity:

```bash
# See overall statistics
python3 migration_helper.py stats

# Analyze a specific controller
python3 migration_helper.py analyze www/app/LightsController.js
python3 migration_helper.py analyze www/app/DashboardController.js
```

**Example output:**
```
============================================================
Angular Migration Statistics
============================================================

LightsController.js              2440 lines   73 .html() calls
ScenesController.js              1147 lines   17 .html() calls
UtilityController.js             1836 lines   14 .html() calls
DashboardController.js           4094 lines   78 .html() calls

------------------------------------------------------------
TOTAL                            9517 lines  182 .html() calls
------------------------------------------------------------

Expected after migration: ~1998 lines
Code reduction: ~7519 lines (79%)
jQuery .html() calls to remove: 182
```

### Step 3: Review Example Implementation

Check out the example Light Widget implementation:

**Files created:**
- `www/app/widgets/lightWidget.js` - Angular directive and controller
- `www/views/widgets/light_widget.html` - Angular template

**Key patterns demonstrated:**

1. **Directive Definition:**
```javascript
app.directive('dzLightWidget', function() {
    return {
        restrict: 'E',
        scope: {
            item: '=',
            config: '='
        },
        templateUrl: 'views/widgets/light_widget.html',
        controller: 'LightWidgetController',
        controllerAs: 'ctrl'
    };
});
```

2. **Controller Logic:**
```javascript
app.controller('LightWidgetController', ['$scope', 'deviceApi',
    function($scope, deviceApi) {
    
    var ctrl = this;
    
    ctrl.toggleDevice = function() {
        deviceApi.switchDevice(item.idx, newStatus);
    };
}]);
```

3. **Template Usage:**
```html
<dz-light-widget ng-repeat="light in ctrl.lights"
                ng-show="ctrl.matchesFilter(light)"
                item="light">
</dz-light-widget>
```

## Tools Provided

### 1. create_github_issue.sh
Helper script to create the GitHub issue.

```bash
./create_github_issue.sh
```

### 2. migration_helper.py
Python script with multiple commands:

```bash
# Show migration statistics
python3 migration_helper.py stats

# Analyze a controller
python3 migration_helper.py analyze <controller_file>

# Generate widget skeleton
python3 migration_helper.py generate-widget Scene
```

### 3. Example Widget Implementation
Reference implementation in `www/app/widgets/` and `www/views/widgets/`.

## Migration Process

### Phase 1: Foundation (2 weeks)
Create reusable widget directives:
- Light widget ✓ (example provided)
- Scene widget
- Utility widget
- Dashboard card widgets

### Phase 2-5: Page Migration (8 weeks)
Convert each page:
1. Create Angular controller
2. Create template with ng-repeat
3. Use widget directives
4. Test thoroughly
5. Deploy with feature flag

### Phase 6: Cleanup (2 weeks)
- Remove jQuery code
- Optimize
- Full testing

## Current Status

✅ **Planning Complete**
- Comprehensive migration plan created
- GitHub issue ready to post
- Example implementation provided
- Migration tools created

⏭️ **Next Steps**
1. Post GitHub issue
2. Get team approval
3. Allocate resources
4. Start Phase 1

## Code Comparison

### Before (jQuery)
```javascript
var html = '<div id="' + item.idx + '">' + item.Name + '</div>';
$('#container').append(html);
$(id + " #name").html(item.Name);
```

### After (Angular)
```javascript
// Controller
ctrl.lights = [];

// Template
<dz-light-widget ng-repeat="light in ctrl.lights"
                item="light">
</dz-light-widget>
```

**Result:** 80% code reduction, no flash bugs, easier to maintain!

## Documentation

- **ANGULAR_MIGRATION_PLAN.md** - Full migration plan (18,660 chars)
- **ANGULAR_MIGRATION_QUICK_REFERENCE.md** - Developer guide (12,683 chars)
- **NEW_GITHUB_ISSUE.md** - GitHub issue content (18,504 chars)

## Questions?

Refer to the detailed documentation or contact the migration team lead.

---

**Ready to start?** Run `./create_github_issue.sh` to begin!
