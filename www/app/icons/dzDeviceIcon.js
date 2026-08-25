define(['app', 'icons/dzIconService'], function (app) {
    'use strict';

    /* <dz-device-icon device="vm.device" is-active="vm.device.isActive()">
       Renders whatever dzIconService resolves for the device: a font glyph
       when there is one, otherwise the PNG the web UI has always used. */
    app.component('dzDeviceIcon', {
        template:
            '<i ng-if="$ctrl.icon.kind === \'font\'" class="{{ $ctrl.icon.cls }}"></i>' +
            '<img ng-if="$ctrl.icon.kind === \'img\'" ng-src="{{ $ctrl.icon.src }}">',
        bindings: {
            device: '<',
            isActive: '<'
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
                    return 'none';
                }

                return [
                    device.Icon,
                    device.CustomImage,
                    device.TypeImg,
                    device.Image,
                    device.SwitchTypeVal,
                    device.Level,
                    device.Status,
                    vm.isActive
                ].join('|');
            }

            function update() {
                signature = currentSignature();
                vm.icon = dzIconService.resolve(vm.device, vm.isActive);
            }
        }]
    });
});
