// Shared by the hardware add/edit form (hardware/Hardware.js) and the Shelly setup page (hardware/setup/Shelly.js):
// reads and validates the optional Gen1 CoIoT push settings and turns them into the tail of an add/updatehardware request.
// The host rules are a strict subset of what the server accepts (ShellyIsValidHost in hardware/ShellyIdentify.cpp, without IPv6),
// so a value that passes here is never refused by the server.
define([], function () {
    var DEFAULT_PERIOD = 15;
    var MIN_PERIOD = 15;
    var MAX_PERIOD = 65535;

    function isPort(s) {
        return /^\d{1,5}$/.test(s) && parseInt(s, 10) <= 65535;
    }

    // Dotted numeric strings must be a real IPv4 address (four parts 0..255, no leading zeros); anything else must be a host name
    // (letters, digits and hyphens per label, no underscores, last label not numeric).
    function isValidHost(host) {
        if (host.length < 1 || host.length > 253) {
            return false;
        }
        if (/^\d+(\.\d+)*$/.test(host)) {
            var parts = host.split('.');
            if (parts.length !== 4) {
                return false;
            }
            for (var i = 0; i < 4; i++) {
                if ((parts[i].length > 1 && parts[i].charAt(0) === '0') || parseInt(parts[i], 10) > 255) {
                    return false;
                }
            }
            return true;
        }
        var name = (host.length > 1 && host.charAt(host.length - 1) === '.') ? host.substring(0, host.length - 1) : host;
        var labels = name.split('.');
        for (var j = 0; j < labels.length; j++) {
            if (!/^[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?$/.test(labels[j])) {
                return false;
            }
        }
        var last = labels[labels.length - 1];
        return !/^\d+$/.test(last) && !/^0[xX]/.test(last);
    }

    // Returns the query string tail ("&address=..&port=..&Mode2=..&Mode3=..") or null (after telling the user) when a value is invalid.
    function getQuery(portStr, addressStr, periodStr, peerPortStr) {
        var port = $.trim(portStr);
        var address = $.trim(addressStr);
        var period = $.trim(periodStr);
        var peerPort = $.trim(peerPortStr);
        if (port === '') port = '0';
        if (peerPort === '') peerPort = '0';
        if (period === '') period = String(DEFAULT_PERIOD);
        if (!isPort(port) || !isPort(peerPort)) {
            ShowNotify($.t('Please enter a valid Port!'), 2500, true);
            return null;
        }
        if (!/^\d{1,5}$/.test(period) || parseInt(period, 10) < MIN_PERIOD || parseInt(period, 10) > MAX_PERIOD) {
            ShowNotify($.t('The CoIoT update period must be between 15 and 65535 seconds'), 2500, true);
            return null;
        }
        if (address !== '' && !isValidHost(address)) {
            ShowNotify($.t('Invalid Domoticz address for device push! Use an IPv4 address or a host name (no IPv6)'), 2500, true);
            return null;
        }
        return '&address=' + encodeURIComponent(address) +
            '&port=' + port +
            '&Mode2=' + period +
            '&Mode3=' + peerPort;
    }

    return {
        DEFAULT_PERIOD: DEFAULT_PERIOD,
        getQuery: getQuery
    };
});
