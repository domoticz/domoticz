define(function() {
    return {
        addTrendData: addTrendData,
        addOneYear: addOneYear,
        formatContractMonthLabel: formatContractMonthLabel
    };

    function addOneYear(isoDate, direction) {
        if (!isoDate || !/^\d{4}-\d{2}-\d{2}$/.test(isoDate)) { return null; }
        var d = new Date(isoDate + 'T00:00:00');
        if (isNaN(d.getTime())) { return null; }
        var origMonth = d.getMonth();
        var origDay   = d.getDate();
        d.setFullYear(d.getFullYear() + (direction || 1));
        if (origMonth === 1 && origDay === 29 && d.getMonth() === 2) { d.setDate(28); }
        return d.getFullYear() + '-'
             + String(d.getMonth() + 1).padStart(2, '0') + '-'
             + String(d.getDate()).padStart(2, '0');
    }

    function formatContractMonthLabel(start, end) {
        var monthNames = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
        var endDay = new Date(end.getTime() - 1);
        return monthNames[start.getMonth()] + ' ' + start.getDate()
             + ' \u2013 ' + monthNames[endDay.getMonth()] + ' ' + endDay.getDate()
             + ' ' + endDay.getFullYear();
    }

    function addTrendData(items, key) {
        return items.map(function (item, index) {
            var trend = 'equal';

            if (index > 0 && item[key] > items[index - 1][key]) {
                trend = 'up'
            }

            if (index > 0 && item[key] < items[index - 1][key]) {
                trend = 'down'
            }

            return Object.assign({}, item, {
                trend: trend
            });
        });
    }
});
