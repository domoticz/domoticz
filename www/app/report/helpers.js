define(function() {
    return {
        addTrendData: addTrendData
    };

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
