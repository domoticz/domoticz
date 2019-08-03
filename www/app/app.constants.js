define(['angular'], function () {
    var module = angular.module('domoticz.constants', []);

    module.constant('dzSettings', {
        serverDateFormat: 'yyyy-MM-dd HH:mm:ss'
    });

    // Default device icon based on SwitchTypeVal ([on, off] states)
    module.constant('dzDefaultSwitchIcons', {
        0: ['Light48_On.png', 'Light48_Off.png'],        // On/Off
        1: ['doorbell48.png', 'doorbell48.png'],         // Doorbell
        2: ['Contact48_On.png', 'Contact48_Off.png'],    // Contact
        3: ['blinds48.png', 'blinds48.png'],             // Blinds,
        4: ['Alarm48_On.png', 'Alarm48_Off.png'],        // X10 Siren
        5: ['smoke48on.png', 'smoke48off.png'],          // Smoke Detector
        6: ['blinds48.png', 'blinds48sel.png'],          // Blinds Inverted
        7: ['Dimmer48_On.png', 'Dimmer48_Off.png'],      // Dimmer
        8: ['motion48-on.png', 'motion48-off.png'],      // Motion Sensor
        9: ['Push48_On.png', 'Push48_Off.png'],          // Push On Button
        10: ['Push48_Off.png', 'Push48_On.png'],         // Push Off Button
        11: ['Door48_On.png', 'Door48_Off.png'],         // Door Contact
        12: ['Water48_On.png', 'Water48_Off.png'],       // Dusk Sensor
        13: ['blinds48.png', 'blinds48.png'],            // Blinds Percentage
        14: ['blinds48.png', 'blinds48.png'],            // Venetian Blinds US
        15: ['blinds48.png', 'blinds48.png'],            // Venetian Blinds EU
        16: ['blinds48.png', 'blinds48.png'],            // Blinds Percentage Inverted
        17: ['Media48_On.png', 'Media48_Off.png'],       // Media Player
        18: ['Generic48_On.png', 'Generic48_Off.png'],   // Selector
        19: ['Door48_On.png', 'Door48_Off.png'],         // Door Lock
        20: ['Door48_On.png', 'Door48_Off.png'],         // Door Lock Inverted
    });

    module.constant('dataTableDefaultSettings', {
        dom: '<"H"lfrC>t<"F"ip>',
        order: [[0, 'desc']],
        "bSortClasses": false,
        "bJQueryUI": true,
        processing: true,
        stateSave: true,
        paging: true,
        pagingType: 'full_numbers',
        pageLength: 25,
        lengthMenu: [[25, 50, 100, -1], [25, 50, 100, "All"]],
        select: {
            className: 'row_selected',
            style: 'single'
        },
        language: $.DataTableLanguage
    });

    return module;
});
