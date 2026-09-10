# Custom dashboard widgets

The dynamic dashboard ("Dashboard 2.0") can load widgets that do not ship with
Domoticz. A theme can bring its own widgets, a Python plugin can ship a widget
for the hardware it talks to, and a widget can be published on its own as a
folder you drop in or clone. None of that requires patching Domoticz.

This document is the contract. It covers `apiVersion` **1**.

- [Where packages live](#where-packages-live)
- [Package layout](#package-layout)
- [The manifest: widget.json](#the-manifest-widgetjson)
- [The widget module](#the-widget-module)
- [Configuration schema](#configuration-schema)
- [Styling](#styling)
- [Talking to Domoticz](#talking-to-domoticz)
- [Lifecycle events](#lifecycle-events)
- [How loading works](#how-loading-works)
- [Rules and limits](#rules-and-limits)
- [Troubleshooting](#troubleshooting)

## Where packages live

Domoticz scans three locations on every `getcustomwidgets` call, so adding or
removing a package needs a browser reload, not a restart:

| Location | Use it for | Served from |
| --- | --- | --- |
| `<www>/widgets/<package>/` | standalone widget repositories, dropped in or cloned | the webroot, directly |
| `<www>/styles/<theme>/widgets/<package>/` | widgets that belong to a theme | the webroot, directly |
| `<userdata>/plugins/<Plugin>/widgets/<package>/` | widgets shipped alongside a Python plugin | the `customwidgetasset` route |

`<www>` is the Domoticz web folder (`www/` in a source checkout) and
`<userdata>` is the folder holding `domoticz.db` and `plugins/`. In a source
checkout they are the same tree; in a packaged install they usually are not.

Only the **active** theme is scanned. Widgets belonging to a theme the user is
not running never show up in the picker.

Plugin packages sit outside the webroot, so their files are served by a
dedicated route rather than the static file handler. This is transparent — your
widget's own assets resolve normally — with one consequence worth knowing: see
[Rules and limits](#rules-and-limits).

## Package layout

A package is a folder with a `widget.json` and the files it names. One package
may declare several widgets.

```
my-widgets/
├── widget.json           required: the manifest
├── powerFlow.widget.js   the widget's javascript
├── powerFlow.html        its template
└── powerFlow.css         optional styles
```

A folder without a `widget.json` is ignored, so it is fine to keep a `README.md`
or a `.git` folder next to it.

## The manifest: widget.json

The manifest describes your widgets to the dashboard. It is deliberately the
*only* thing read at startup: because the descriptor lives here rather than in
your javascript, Domoticz can list your widget in the picker without executing
any of your code.

```json
{
    "name": "My Widgets",
    "description": "Power flow visualisation",
    "author": "your name",
    "version": "1.0.0",
    "apiVersion": 1,
    "widgets": [
        {
            "type": "power-flow",
            "label": "Power Flow",
            "description": "Live import/export flow between grid, solar and house",
            "category": "Energy",
            "icon": "fa-solid fa-bolt",
            "entry": "powerFlow.widget.js",
            "template": "powerFlow.html",
            "css": "powerFlow.css",
            "defaultW": 4,
            "defaultH": 3,
            "minW": 2,
            "minH": 2,
            "maxW": 12,
            "maxH": 6,
            "transparentBackground": false,
            "configSchema": []
        }
    ]
}
```

### Package fields

| Field | Required | Meaning |
| --- | --- | --- |
| `apiVersion` | yes | Must be `1`. A package declaring anything else is ignored with a log line, which is how a future breaking change stays safe. |
| `widgets` | yes | Non-empty array of widget descriptors. |
| `name` | no | Human-readable package name, shown as the widget's provider. |
| `description` | no | What the package is for. |
| `author` | no | Shown as the widget's provider. |
| `version` | no | Your version string; Domoticz does not interpret it. |

### Widget descriptor fields

| Field | Required | Meaning |
| --- | --- | --- |
| `type` | yes | Unique id for this widget. Persisted in the user's dashboard, so **never change it** after release. Must start with a letter and hold only letters, digits, `-` and `_`. |
| `label` | yes | Name in the widget picker and the card header. |
| `entry` | yes | Your javascript file, relative to the package folder. |
| `description` | no | One line in the picker; also searched by the picker's filter box. |
| `category` | no | Picker grouping. Known ones: `Devices`, `Energy`, `Controls`, `Charts & Data`, `Weather`, `Information`, `Custom Content`, `System`. Anything else becomes its own group at the end. |
| `icon` | no | Font Awesome classes, e.g. `fa-solid fa-bolt`. |
| `template` | no | Template file, relative to the package folder. Optional only if your module supplies its own `template` or `templateUrl`. |
| `css` | no | Stylesheet, relative to the package folder. Injected once, on first render. |
| `defaultW` / `defaultH` | no | Size in grid units when first placed. |
| `minW` / `minH` / `maxW` / `maxH` | no | Resize limits. |
| `transparentBackground` | no | `true` drops the card's panel background — for clock-like widgets. |
| `configSchema` | no | The widget's settings form; see below. |

Anything else you add is passed through to the descriptor untouched, so a future
Domoticz can grow new descriptor fields without a change here.

Relative paths (`entry`, `template`, `css`) must stay inside the package: no
absolute paths, no `..`, no scheme. Domoticz rejects a widget that breaks this.

## The widget module

`entry` is an AMD module (the dashboard uses RequireJS) that exports an Angular
[directive definition object][ddo]. Domoticz registers the directive itself, so
your module must **not** depend on `app` or register anything globally.

[ddo]: https://docs.angularjs.org/api/ng/service/$compile#directive-definition-object

Two forms are accepted. Export a factory when the widget needs to know where its
package lives:

```js
define(function() {
    'use strict';

    // ctx.type        this widget's type
    // ctx.baseUrl     url of the package folder; append your own asset names
    // ctx.templateUrl the manifest's "template", already resolved (null if unset)
    return function(ctx) {
        return {
            templateUrl: ctx.templateUrl,
            controller: ['$scope', '$http', function($scope, $http) {
                var ctrl = this;
                ctrl.logo = ctx.baseUrl + 'logo.svg';
            }]
        };
    };
});
```

Or export the definition object directly when it does not:

```js
define(function() {
    'use strict';

    return {
        template: '<div class="my-widget">{{ ctrl.text }}</div>',
        controller: ['$scope', function($scope) {
            this.text = 'hello';
        }]
    };
});
```

Your module may pull in its own dependencies through `define([...])` as usual,
including Domoticz services registered on the injector.

### What Domoticz fills in

Unless your definition object says otherwise, you get:

```js
{
    scope: { widgetDef: '=', editMode: '<' },
    controllerAs: 'ctrl',
    bindToController: true
}
```

so `ctrl.widgetDef` and `ctrl.editMode` are available in the controller and
`ctrl.` is the template prefix. If you declare your own `scope`, it replaces
this one wholesale — keep `widgetDef` and `editMode` in it.

`restrict` is always forced to `'E'`; the dashboard renders widgets as elements.

If neither your object nor the manifest supplies a template, the widget fails to
load with an error on the card.

### The two bindings

`widgetDef` is the placed widget. The part you care about is `widgetDef.config`,
holding the user's answers to your `configSchema`:

```js
function config() {
    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
}
```

Read it through a helper like that rather than caching it. The user can open the
settings dialog while your widget is on screen, and the dashboard applies the
result by mutating `config` in place — so watch it if you need to react:

```js
$scope.$watch(function() { return config(); }, applyConfig, true);
```

`editMode` is `true` while the dashboard is being edited. Use it to suppress
interactions that would fight with dragging and resizing.

## Configuration schema

Each entry in `configSchema` renders one field in the widget's settings dialog.

```json
{ "key": "deviceIdx", "type": "device-picker", "label": "Device", "required": true }
```

Common keys: `key` (where the value lands in `config`), `type`, `label`,
`default`, `required`, `help`. Numeric fields also take `min` and `max`;
`picker` and `select` take `options` as `[{ "value": …, "label": … }]`.

Available `type` values:

| Type | Field |
| --- | --- |
| `text`, `textarea`, `url` | free text |
| `number`, `range` | numeric input, slider |
| `boolean` | toggle |
| `color`, `color-alpha` | colour picker, with or without alpha |
| `select`, `picker` | dropdown over `options` |
| `device-picker`, `device-list` | one or many Domoticz devices |
| `scene-picker`, `camera-picker`, `plan-picker` | one scene / camera / room plan |
| `action-list`, `range-list` | repeating rows |
| `group` | visual grouping of the fields that follow |

A widget with no `configSchema` has no settings dialog, and the "configure"
action is hidden on its card.

## Styling

The `css` file is injected once, the first time one of the package's widgets is
rendered. It is a plain global stylesheet: nothing scopes it to your widget, so
**prefix every selector** with something specific to your package.

Use the Domoticz theme variables instead of literal colours so your widget
follows the user's theme, including third-party ones:

```css
.my-widget__value  { color: var(--dz-accent-color, #4e9af1); }
.my-widget__label  { color: var(--dz-body-text); }
.my-widget--faded  { opacity: 0.6; }
```

Your widget is rendered inside the card body, which is a flex column. Give your
root element `height: 100%` and `box-sizing: border-box` if you want to fill it.

## Talking to Domoticz

Use the JSON API through `$http`, exactly as the built-in widgets do:

```js
$http.get('json.htm', { params: { type: 'command', param: 'getdevices', rid: idx } })
    .then(function(resp) { ctrl.device = (resp.data.result || [])[0]; });
```

Domoticz already holds a websocket to the browser, so prefer reacting to its
broadcasts over polling:

| Event | Fired when |
| --- | --- |
| `device_update` | a device changed; the payload is the device, so filter on `idx` |
| `time_update` | the server clock ticked; carries `serverTime` and `actTime` |

```js
$scope.$on('device_update', function(event, updated) {
    if (ctrl.device && String(updated.idx) === String(ctrl.device.idx)) {
        ctrl.device = angular.extend({}, ctrl.device, updated);
    }
});
```

Injectable services worth knowing: `$http`, `$q`, `$interval`, `$timeout`,
`livesocket`, `domoticzApi`, `dzTimeAndSun`, `ddVisibility`, `ddToast`.

## Lifecycle events

| Event | Meaning |
| --- | --- |
| `dd:widget:refresh` | the user asked the dashboard to refresh — re-read your data |
| `dd:page:hidden` | the dashboard is no longer visible — stop timers |
| `dd:page:visible` | it is visible again — redraw and restart timers |
| `$destroy` | the widget is going away — cancel intervals, drop listeners |

Honouring the visibility pair matters: a dashboard left open on a wall tablet
will otherwise keep every widget's timers running for days.

```js
var timer = null;
function stop()  { if (timer) { $interval.cancel(timer); timer = null; } }
function start() { stop(); timer = $interval(tick, 1000); }

$scope.$on('dd:page:hidden',  stop);
$scope.$on('dd:page:visible', function() { tick(); start(); });
$scope.$on('$destroy',        stop);
```

## How loading works

Worth understanding, because it explains the constraints:

1. On dashboard load, the browser calls `getcustomwidgets`. Domoticz scans the
   three locations, validates each `widget.json`, and returns the packages.
2. Every descriptor is registered into the widget registry. **No third-party
   code has run yet** — this is why a broken package cannot keep the picker, or
   the rest of the dashboard, from working.
3. The first time a widget of your type is actually rendered, its `css` is
   injected and its `entry` is fetched. Your directive is registered, and the
   card compiles. Later instances reuse it.
4. While step 3 is in flight the card shows a spinner. If it fails, the card
   shows the reason and the rest of the dashboard is unaffected.

## Rules and limits

**Pick a `type` and keep it.** It is stored in every dashboard that uses your
widget. Changing it orphans those cards. Prefix it to stay clear of other
packages: `acme-power-flow` rather than `power`.

**Built-in widgets always win.** If your `type` matches a stock widget, or one
another package already claimed, yours is skipped with a console warning. The
first package discovered keeps the name.

**Element names cannot collide.** Domoticz derives the directive name from your
type itself, so you never pick an element name and can never clash with a
built-in one.

**Your code is not sandboxed.** It runs with the same access as Domoticz's own
javascript. That is a deliberate choice — a Python plugin already runs arbitrary
code on the server, and `www/templates/` has always allowed custom Angular
pages — but it means installing a widget package is as much a trust decision as
installing a plugin. Read what you install.

**Plugin packages: build asset urls from `ctx.baseUrl`.** Files under
`plugins/…/widgets/` are served through a query-based route, so a relative
`<img src="logo.svg">` inside your template will not resolve. Use
`ctx.baseUrl + 'logo.svg'`. Widgets in `www/widgets/` and theme widgets are
served as ordinary paths and do not have this constraint, but using `baseUrl`
everywhere keeps a package portable between the three locations.

**Allowed asset extensions.** For plugin packages, only `js`, `html`, `css`,
`json`, `png`, `jpg`, `jpeg`, `gif`, `svg`, `webp`, `woff` and `woff2` are
served. Others get a 403.

**A manifest is capped at 256 KB** and a widget's `type` at 64 characters.

## Troubleshooting

**Finding your widget in the picker.** Installed widgets sit in whatever
category their manifest declares, badged with the package they came from. The
Widget Library's **Custom** filter narrows the list to package widgets only,
and its search box matches package, theme, plugin and author names as well as
widget labels.

**The widget is not in the picker.** Call
`json.htm?type=command&param=getcustomwidgets` directly and see whether your
package is listed. If it is not, check the Domoticz log: every rejected package
is logged with the reason (bad JSON, wrong `apiVersion`, no `widgets` array,
unsafe asset path). Confirm the folder holds a `widget.json`, and for a theme
package, that the theme is the active one.

**The card says "failed to load".** The browser console has the detail. Usual
causes: the `entry` file 404s, the module throws while loading, or it returned
something that is not a directive definition object.

**The card says "Unknown widget type".** A dashboard references a widget whose
package is no longer installed, or was renamed. Reinstall it, or delete the
card.

**Settings changes do not show up.** You cached `widgetDef.config` instead of
reading it through a helper, or you are watching it without `true` for deep
comparison.

## See also

- [`docs/dashboardDynamic.md`](dashboardDynamic.md) — the dashboard itself
- [`docs/Theming.wiki`](Theming.wiki) — building a theme
- [`docs/Developing_a_Python_plugin.wiki`](Developing_a_Python_plugin.wiki) — plugins
- `www/widgets/example/` — a working reference package
