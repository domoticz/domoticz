define(['app', 'icons/dzIconService'], function (app) {
    'use strict';

    /* <dz-device-icon device="vm.device" is-active="vm.device.isActive()">
       Renders whatever dzIconService resolves for the device: a font glyph
       when there is one, otherwise the PNG the web UI has always used.

       The host element carries the sizing, through one of the dz-icon-* classes
       in style.css, because the same widget is drawn at 16, 40 and 48 pixels. */
    app.component('dzDeviceIcon', {
        template:
            '<i ng-if="$ctrl.icon.kind === \'font\'" class="{{ $ctrl.glyphClass }}"></i>' +
            '<img ng-if="$ctrl.icon.kind === \'img\'" ng-src="{{ $ctrl.icon.src }}" class="{{ $ctrl.stateClass }}">',
        bindings: {
            device: '<',
            isActive: '<',
            /* The PNG this call site would have built for itself. Preferred over
               the resolver's own legacy image, because several widgets know more
               about their device than dzIconService does - the selected state of
               a blind, a media player that is disconnected, the exact icon root
               a counter uses - and their PNG must keep looking as it does now. */
            fallbackSrc: '<',
            /* Pass false where the PNG encodes a value that one glyph per device
               type cannot carry: the alert level, the wind direction, the
               temperature range. Those keep the image, and only an icon chosen
               explicitly for that device still wins over it. */
            useGlyph: '<'
        },
        controller: ['dzIconService', function (dzIconService) {
            var vm = this;
            var signature = null;

            vm.$onInit = init;
            vm.$onChanges = update;
            vm.$doCheck = checkForChanges;

            function init() {
                update();
                /* The built-in icon set is fetched once, asynchronously. Resolve
                   again when it lands: nothing about the device changed, so
                   $doCheck would not notice on its own. */
                dzIconService.preloadBuiltinIcons().then(update);
            }

            /* The device object is usually mutated in place when a state
               update arrives over the websocket, so $onChanges alone would not
               fire. Watch the few fields the icon depends on instead of
               re-resolving on every digest. */
            function checkForChanges() {
                if (currentSignature() !== signature) {
                    update();
                }
            }

            function currentSignature() {
                var device = vm.device;
                if (!device) {
                    return ['none', vm.fallbackSrc, vm.useGlyph].join('|');
                }

                return [
                    device.Icon,
                    device.CustomImage,
                    device.TypeImg,
                    device.Image,
                    device.SwitchTypeVal,
                    device.Level,
                    device.Status,
                    vm.isActive,
                    vm.fallbackSrc,
                    vm.useGlyph
                ].join('|');
            }

            /* true, false, or null when neither the call site nor the device can
               say. dzIconService keeps its own copy of the "is this device on"
               rule for plain JSON devices; repeating it here would be a second
               place to keep in step, and guessing is worse than saying nothing:
               a temperature sensor is neither on nor off. */
            function activeFlag() {
                if (vm.isActive !== undefined && vm.isActive !== null) {
                    return !!vm.isActive;
                }
                if (vm.device && typeof vm.device.isActive === 'function') {
                    return !!vm.device.isActive();
                }
                return null;
            }

            function update() {
                signature = currentSignature();

                var active = activeFlag();
                vm.icon = resolveIcon(active);

                /* The glyph set has one icon per device type, so on and off would
                   look identical without a hook for the stylesheet to work with.
                   Emitted on the image too, so a theme can style both the same
                   way, even though the default rule only touches the glyph. */
                vm.stateClass = active === null ? '' : (active ? 'dz-icon--on' : 'dz-icon--off');
                vm.glyphClass = vm.icon.kind === 'font'
                    ? ('dz-icon-glyph ' + vm.icon.cls + (vm.stateClass ? ' ' + vm.stateClass : ''))
                    : '';
            }

            function resolveIcon(active) {
                var resolved = dzIconService.resolve(vm.device, vm.isActive);

                if (vm.useGlyph === false) {
                    /* Only an icon chosen for this device by hand outranks a
                       value-driven image, so ask for that one step on its own.
                       The resolver's own src is unset when it found a glyph,
                       hence the guard on the fallback. */
                    var explicit = dzIconService.resolveIconClass(vm.device && vm.device.Icon, active);
                    if (explicit) {
                        return { kind: 'font', cls: explicit };
                    }

                    return {
                        kind: 'img',
                        src: vm.fallbackSrc || (resolved.kind === 'img' ? resolved.src : 'images/unknown.png')
                    };
                }

                if (resolved.kind === 'img' && vm.fallbackSrc) {
                    return { kind: 'img', src: vm.fallbackSrc };
                }

                return resolved;
            }
        }]
    });
});
