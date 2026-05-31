define(function() {
    return {
        addTrendData: addTrendData,
        addOneYear: addOneYear,
        formatContractMonthLabel: formatContractMonthLabel,
        exportTableToExcel: exportTableToExcel,
        exportTableToCSV: exportTableToCSV,
        exportTableToClipboard: exportTableToClipboard
    };

    function exportTableToExcel(element, filename) {
        function escXml(s) {
            return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
                    .replace(/"/g, '&quot;').replace(/'/g, '&apos;');
        }
        function xmlCell(text) {
            var t = text.trim();
            var isNum = /^-?\d+(\.\d+)?$/.test(t);
            return '<Cell><Data ss:Type="' + (isNum ? 'Number' : 'String') + '">' + escXml(t) + '</Data></Cell>';
        }
        var xml = '<?xml version="1.0" encoding="UTF-8"?>\n'
            + '<?mso-application progid="Excel.Sheet"?>\n'
            + '<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet"'
            + ' xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet">\n'
            + '<Worksheet ss:Name="Report"><Table>\n';
        xml += '<Row>';
        element.find('#reporttable thead th').each(function () { xml += xmlCell($(this).text()); });
        xml += '</Row>\n';
        element.find('#reporttable tbody tr, #reporttable tfoot tr').each(function () {
            xml += '<Row>';
            $(this).find('td').each(function () { xml += xmlCell($(this).text()); });
            xml += '</Row>\n';
        });
        xml += '</Table></Worksheet></Workbook>';
        var blob = new Blob([xml], { type: 'application/vnd.ms-excel' });
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a'); a.href = url; a.download = filename + '.xls'; a.click();
        URL.revokeObjectURL(url);
    }

    function exportTableToCSV(element, filename) {
        var rows = [];
        var headers = [];
        element.find('#reporttable thead th').each(function () { headers.push($(this).text()); });
        rows.push(headers.join(','));
        element.find('#reporttable tbody tr').each(function () {
            var cols = [];
            $(this).find('td').each(function () {
                var val = $(this).text().replace(/"/g, '""');
                // Prevent formula injection in spreadsheet applications
                if (/^[=+\-@]/.test(val)) { val = "'" + val; }
                cols.push('"' + val + '"');
            });
            rows.push(cols.join(','));
        });
        element.find('#reporttable tfoot tr').each(function () {
            var cols = [];
            $(this).find('td').each(function () {
                var val = $(this).text().replace(/"/g, '""');
                // Prevent formula injection in spreadsheet applications
                if (/^[=+\-@]/.test(val)) { val = "'" + val; }
                cols.push('"' + val + '"');
            });
            rows.push(cols.join(','));
        });
        var blob = new Blob([rows.join('\n')], { type: 'text/csv' });
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a'); a.href = url; a.download = filename + '.csv'; a.click();
        URL.revokeObjectURL(url);
    }

    function exportTableToClipboard(element) {
        var rows = [];
        var headers = [];
        element.find('#reporttable thead th').each(function () { headers.push($(this).text()); });
        rows.push(headers.join('\t'));
        element.find('#reporttable tbody tr').each(function () {
            var cols = [];
            $(this).find('td').each(function () {
                var val = $(this).text();
                // Prevent formula injection in spreadsheet applications
                if (/^[=+\-@]/.test(val)) { val = "'" + val; }
                cols.push(val);
            });
            rows.push(cols.join('\t'));
        });
        element.find('#reporttable tfoot tr').each(function () {
            var cols = [];
            $(this).find('td').each(function () {
                var val = $(this).text();
                // Prevent formula injection in spreadsheet applications
                if (/^[=+\-@]/.test(val)) { val = "'" + val; }
                cols.push(val);
            });
            rows.push(cols.join('\t'));
        });
        navigator.clipboard.writeText(rows.join('\n'));
    }

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
