define(['app'], function(app) {
    'use strict';

    app.directive('ddWcpColor', [function() {
        return {
            restrict:  'E',
            require:   'ngModel',
            template:  '<button type="button" class="dd-wcp-swatch"></button>' +
                       '<div class="dd-wcp-drop" style="display:none">' +
                       '  <input type="text">' +
                       '</div>',
            link: function(scope, element, attrs, ngModel) {
                var btn   = element.find('button');
                var drop  = element.find('.dd-wcp-drop');
                var input = element.find('input');
                var isOpen = false;

                input.wheelColorPicker({
                    layout:  'block',
                    sliders: 'wva',
                    format:  'rgba'
                });

                function applyValue(val) {
                    if (val) {
                        input.wheelColorPicker('setValue', val);
                    }
                    btn.css('background', val || 'var(--dz-accent-color)');
                    btn.toggleClass('dd-wcp-swatch--auto', !val);
                }

                function open() {
                    drop.show();
                    isOpen = true;
                    // Apply AFTER the block picker is visible so height() is non-zero
                    // and the alpha cursor lands in the correct position.
                    var val = ngModel.$modelValue;
                    if (val) {
                        input.wheelColorPicker('setValue', val);
                        input.wheelColorPicker('refreshWidget');
                        input.wheelColorPicker('updateSliders');
                    }
                }

                function close() {
                    drop.hide();
                    isOpen = false;
                }

                btn.on('click', function(e) {
                    e.stopPropagation();
                    isOpen ? close() : open();
                });

                $(document).on('click.ddWcp' + scope.$id, function(e) {
                    if (isOpen && !element[0].contains(e.target)) {
                        close();
                    }
                });

                ngModel.$render = function() {
                    applyValue(ngModel.$viewValue);
                };

                input.on('change.wheelColorPicker', function() {
                    var val = input.wheelColorPicker('getValue', 'rgba');
                    btn.css('background', val);
                    scope.$apply(function() {
                        ngModel.$setViewValue(val);
                    });
                });

                scope.$on('$destroy', function() {
                    input.off('change.wheelColorPicker');
                    $(document).off('click.ddWcp' + scope.$id);
                });
            }
        };
    }]);
});
