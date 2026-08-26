define(['app', 'icons/dzIconService'], function (app) {
    'use strict';

    app.component('dzDeviceIcon', {
        template:
            '<i ng-if="$ctrl.icon.kind === \'font\'" class="{{ $ctrl.glyphClass }}"></i>' +
            '<img ng-if="$ctrl.icon.kind === \'img\'" ng-src="{{ $ctrl.icon.src }}" class="{{ $ctrl.stateClass }}">',
        bindings: {
            device: '<',
            isActive: '<',
            fallbackSrc: '<',
            // false where the PNG encodes a reading, not a device: alert level, wind direction, temperature range.
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
                dzIconService.preloadBuiltinIcons().then(update);
            }

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

                vm.stateClass = active === null ? '' : (active ? 'dz-icon--on' : 'dz-icon--off');
                vm.glyphClass = vm.icon.kind === 'font'
                    ? ('dz-icon-glyph ' + vm.icon.cls + (vm.stateClass ? ' ' + vm.stateClass : ''))
                    : '';
            }

            function resolveIcon(active) {
                var resolved = dzIconService.resolve(vm.device, vm.isActive);

                if (vm.useGlyph === false) {
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
