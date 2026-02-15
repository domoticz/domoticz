# Quick Reference: Angular Migration Guide

## Overview

This guide provides quick reference information for developers working on the Angular migration of jQuery-based pages.

## Architecture Comparison

### jQuery Approach (Old ❌)

```javascript
// Generate HTML as strings
var html = '<div id="' + item.idx + '">' + item.Name + '</div>';
$('#container').append(html);

// Manual DOM updates
$(id + " #name").html(item.Name);
$(id + " #status").html(item.Status);

// Manual filtering
$('.itemBlock').each(function() {
    if (matches) {
        $(this).show();
    } else {
        $(this).hide();
    }
});
```

**Problems:**
- String concatenation is error-prone
- Manual DOM updates are tedious
- Race conditions with Angular
- Hard to test
- Performance issues

### Angular Approach (New ✅)

```javascript
// Controller
ctrl.items = [];
ctrl.searchQuery = '';

ctrl.matchesFilter = function(item) {
    if (!ctrl.searchQuery) return true;
    return item.searchText.toLowerCase()
        .indexOf(ctrl.searchQuery.toLowerCase()) !== -1;
};

// Updates happen automatically via data binding
$scope.$on('device_update', function(event, device) {
    ctrl.items.forEach(function(item, index) {
        if (item.idx === device.idx) {
            ctrl.items[index] = device; // Angular handles the rest!
        }
    });
});
```

```html
<!-- Template -->
<input ng-model="ctrl.searchQuery">

<dz-widget ng-repeat="item in ctrl.items"
          ng-show="ctrl.matchesFilter(item)"
          item="item">
</dz-widget>
```

**Benefits:**
- Declarative templates
- Automatic updates
- No race conditions
- Easy to test
- Better performance

## Widget Directive Template

```javascript
// www/app/widgets/myWidget.js
angular.module('app').directive('dzMyWidget', function() {
    return {
        restrict: 'E',
        scope: {
            item: '=',           // Device data (two-way binding)
            config: '=',         // Configuration object
            ordering: '=',       // Enable drag-drop
            onUpdate: '&'        // Callback function
        },
        templateUrl: 'views/widgets/my_widget.html',
        controller: 'MyWidgetController',
        controllerAs: 'ctrl',
        link: function(scope, element, attrs) {
            // DOM-specific logic only (if needed)
            // Example: tooltips, drag-drop initialization
        }
    };
});

angular.module('app').controller('MyWidgetController', 
    function($scope, $element, deviceApi) {
    
    var ctrl = this;
    var item = $scope.item;
    
    // Computed properties
    ctrl.getStatusText = function() {
        return item.Status === 'On' ? 'Active' : 'Inactive';
    };
    
    ctrl.getBackgroundClass = function() {
        return 'status' + item.Status;
    };
    
    // Actions
    ctrl.toggleDevice = function() {
        deviceApi.toggleDevice(item.idx).then(function() {
            // Update will come via device_update event
        });
    };
    
    ctrl.editDevice = function() {
        // Open edit dialog
    };
});
```

```html
<!-- views/widgets/my_widget.html -->
<div class="item" ng-class="ctrl.getBackgroundClass()">
    <section>
        <table>
            <tr>
                <td id="name">{{item.Name}}</td>
                <td id="status">{{ctrl.getStatusText()}}</td>
                <td id="lastupdate">{{item.LastUpdate}}</td>
                <td class="options">
                    <a ng-click="ctrl.toggleDevice()">Toggle</a>
                    <a ng-click="ctrl.editDevice()">Edit</a>
                </td>
            </tr>
        </table>
    </section>
</div>
```

## Page Controller Template

```javascript
// www/app/MyPageController.js
angular.module('app').controller('MyPageController', 
    function($scope, $rootScope, $routeParams, deviceApi, livesocket) {
    
    var ctrl = this;
    
    // === 1. STATE ===
    ctrl.items = [];
    ctrl.searchQuery = '';
    
    // === 2. FILTER FUNCTIONS ===
    ctrl.matchesFilter = function(item) {
        if (!ctrl.searchQuery || ctrl.searchQuery.length === 0) {
            return true;
        }
        
        var searchText = item.searchText || '';
        var query = ctrl.searchQuery.toLowerCase();
        
        // Support multi-word search
        var searchTerms = query.split(/\,|\s/).filter(function(term) {
            return term.length > 0;
        });
        
        return searchTerms.every(function(term) {
            return searchText.toLowerCase().indexOf(term) !== -1;
        });
    };
    
    ctrl.getFilteredCount = function() {
        if (!ctrl.items) return 0;
        return ctrl.items.filter(ctrl.matchesFilter).length;
    };
    
    // === 3. ACTIONS ===
    ctrl.toggleDevice = function(device) {
        deviceApi.toggleDevice(device.idx);
    };
    
    ctrl.editDevice = function(device) {
        // Open edit dialog
    };
    
    // === 4. EVENT LISTENERS ===
    $scope.$on('device_update', function(event, deviceData) {
        updateItem(deviceData);
    });
    
    // === 5. PRIVATE FUNCTIONS ===
    function updateItem(item) {
        item.searchText = generateSearchText(item);
        
        ctrl.items.forEach(function(olditem, index) {
            if (olditem.idx === item.idx) {
                ctrl.items[index] = item;
            }
        });
    }
    
    function generateSearchText(item) {
        return [
            item.Name,
            item.Description,
            item.Type,
            item.SubType,
            item.idx
        ].join(' ').toLowerCase();
    }
    
    function loadItems() {
        var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
        
        livesocket.getJson(
            "json.htm?type=command&param=getdevices&filter=mytype&used=true&plan=" + roomPlanId,
            function(data) {
                if (data.result) {
                    data.result.forEach(function(item) {
                        item.searchText = generateSearchText(item);
                    });
                    ctrl.items = data.result;
                } else {
                    ctrl.items = [];
                }
            }
        );
    }
    
    // === 6. INITIALIZATION ===
    function init() {
        loadItems();
    }
    
    init();
});
```

## Page Template

```html
<!-- views/my_page.html -->
<div class="container">
    <!-- Top bar with search -->
    <div ng-include="'views/inc_topbar.html'"></div>
    
    <!-- Main content -->
    <div class="devicesList">
        <div class="row divider">
            <!-- No items message -->
            <h2 ng-show="ctrl.items.length == 0" 
                data-i18n="No items found...">
                No items found...
            </h2>
            
            <!-- Grid layout helper (for 3-column layout) -->
            <div ng-repeat-start="item in ctrl.items" 
                 class="clearfix" 
                 ng-if="$index % 3 == 0 && !$first && ctrl.matchesFilter(item)">
            </div>
            
            <!-- Widget -->
            <dz-my-widget ng-repeat-end
                         class="span4 itemBlock"
                         id="{{item.idx}}"
                         ng-show="ctrl.matchesFilter(item)"
                         item="item"
                         config="config"
                         ordering="config.AllowWidgetOrdering">
            </dz-my-widget>
        </div>
    </div>
</div>
```

## Common Patterns

### 1. Device Toggle

```javascript
// In widget controller
ctrl.toggleDevice = function() {
    deviceApi.switchDevice(item.idx, item.Status === 'On' ? 'Off' : 'On')
        .then(function(response) {
            // Update will come via device_update event
            // No need to manually update DOM!
        });
};
```

### 2. Favorites

```javascript
ctrl.toggleFavorite = function() {
    deviceApi.makeFavorite(item.idx, item.Favorite ? 0 : 1)
        .then(function() {
            // Reload or update via event
        });
};
```

### 3. Filtering with Room Plans

```javascript
ctrl.RoomPlans = $rootScope.GetRoomPlans();
ctrl.roomSelected = $routeParams.room || window.myglobals.LastPlanSelected;

ctrl.changeRoom = function() {
    window.myglobals.LastPlanSelected = ctrl.roomSelected;
    $route.updateParams({
        room: ctrl.roomSelected >= 0 ? ctrl.roomSelected : undefined
    });
    $location.replace();
};
```

### 4. Real-time Updates

```javascript
// In controller init
$scope.$on('device_update', function(event, deviceData) {
    deviceData.searchText = generateSearchText(deviceData);
    
    var found = false;
    ctrl.items.forEach(function(item, index) {
        if (item.idx === deviceData.idx) {
            ctrl.items[index] = deviceData;
            found = true;
        }
    });
    
    // If new device, add it
    if (!found && deviceData.used) {
        ctrl.items.push(deviceData);
    }
});
```

## Testing

### Unit Test Template

```javascript
describe('MyWidgetController', function() {
    var $controller, $scope, ctrl;
    
    beforeEach(module('app'));
    
    beforeEach(inject(function(_$controller_, $rootScope) {
        $scope = $rootScope.$new();
        $scope.item = {
            idx: 1,
            Name: 'Test Device',
            Status: 'Off'
        };
        
        ctrl = _$controller_('MyWidgetController', {
            $scope: $scope
        });
    }));
    
    it('should initialize correctly', function() {
        expect(ctrl).toBeDefined();
    });
    
    it('should toggle device status', function() {
        expect($scope.item.Status).toBe('Off');
        ctrl.toggleDevice();
        // Test API call was made
    });
    
    it('should compute status text correctly', function() {
        $scope.item.Status = 'On';
        expect(ctrl.getStatusText()).toBe('Active');
        
        $scope.item.Status = 'Off';
        expect(ctrl.getStatusText()).toBe('Inactive');
    });
});
```

## Migration Checklist

For each page:

### Analysis Phase
- [ ] List all device types shown on this page
- [ ] Document all user interactions (click, drag, etc.)
- [ ] Document all real-time update behaviors
- [ ] Identify special features or edge cases
- [ ] Count number of `.html()` calls

### Design Phase
- [ ] Design widget directive(s) needed
- [ ] Design controller structure
- [ ] Design template layout
- [ ] Plan filtering implementation
- [ ] Plan testing strategy

### Implementation Phase
- [ ] Create widget directive(s)
- [ ] Create widget template(s)
- [ ] Create page controller
- [ ] Create page template
- [ ] Implement filtering
- [ ] Implement real-time updates
- [ ] Implement user interactions

### Testing Phase
- [ ] Write unit tests
- [ ] Manual testing in development
- [ ] Test filtering
- [ ] Test real-time updates
- [ ] Test device controls
- [ ] Test on mobile
- [ ] Performance testing

### Deployment Phase
- [ ] Code review
- [ ] Deploy to staging
- [ ] User acceptance testing
- [ ] Deploy to production
- [ ] Monitor for issues

## Common Pitfalls

### ❌ Don't: Manual DOM Manipulation
```javascript
// BAD
$(id + " #name").html(item.Name);
```

### ✅ Do: Use Data Binding
```html
<!-- GOOD -->
<td id="name">{{item.Name}}</td>
```

### ❌ Don't: jQuery Selectors in Controller
```javascript
// BAD
$('#lightcontent').append(html);
```

### ✅ Do: Let ng-repeat Handle It
```html
<!-- GOOD -->
<div ng-repeat="light in ctrl.lights">
```

### ❌ Don't: setTimeout Workarounds
```javascript
// BAD
setTimeout(function() {
    RefreshLiveSearch();
}, 0);
```

### ✅ Do: Let Angular Digest Handle It
```javascript
// GOOD - Angular automatically updates!
ctrl.items[index] = newData;
```

## Performance Tips

1. **Use track by in ng-repeat**
   ```html
   <div ng-repeat="item in ctrl.items track by item.idx">
   ```

2. **Use one-time binding for static data**
   ```html
   <td>{{::item.idx}}</td>
   ```

3. **Minimize watchers**
   - Use `ng-if` instead of `ng-show` when possible (removes from DOM)
   - Avoid complex expressions in templates
   - Use controller functions for complex logic

4. **Debounce search**
   ```javascript
   ctrl.searchQuery = '';
   var searchTimeout;
   
   $scope.$watch('ctrl.searchQuery', function(newVal) {
       if (searchTimeout) $timeout.cancel(searchTimeout);
       searchTimeout = $timeout(function() {
           // Search logic
       }, 300);
   });
   ```

## Resources

- [ANGULAR_MIGRATION_PLAN.md](./ANGULAR_MIGRATION_PLAN.md) - Full migration plan
- Temperature Controller - Reference implementation
- Weather Controller - Reference implementation
- [AngularJS Docs](https://docs.angularjs.org/)

## Questions?

Contact the migration team lead or post in the development channel.
