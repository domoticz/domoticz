define(function() {
    function ChartWatermark() {

    }

    function renderWatermark(chart, configName, elementName, cssClass) {
        var config = chart[configName];
        if (!config || !config.visible) {
            if (chart[elementName]) {
                chart[elementName].destroy();
                chart[elementName] = null;
            }
            return;
        }
        var maxWidth = Math.round(chart.plotWidth * 0.8);
        var x = chart.plotLeft + chart.plotWidth / 2;
        var y = chart.plotTop;
        if (chart[elementName]) {
            chart[elementName].attr({
                text: '<div class="wm-message" style="maxWidth:' + maxWidth + 'px">' + config.text + '</div>',
                x: x,
                y: y
            });
            return;
        }
        chart[elementName] = chart.renderer
            .label('<div class="wm-message" style="maxWidth:' + maxWidth + 'px">' + config.text + '</div>', x, y, null, null, null, true)
            .addClass(cssClass)
            .attr({ align: 'center', zIndex: 5 })
            .add();
    }

    function showError(chart, errorMessage) {
        chart.watermarkErrorConfig.text = errorMessage;
        chart.watermarkWarningConfig.visible = false;
        renderWatermark(chart, 'watermarkWarningConfig', 'watermarkWarning', 'chart-watermark-warning');
        chart.watermarkErrorConfig.visible = true;
        renderWatermark(chart, 'watermarkErrorConfig', 'watermarkError', 'chart-watermark-error');
    }

    function showWarning(chart, warningMessage) {
        chart.watermarkWarningConfig.text = warningMessage;
        chart.watermarkErrorConfig.visible = false;
        renderWatermark(chart, 'watermarkErrorConfig', 'watermarkError', 'chart-watermark-error');
        chart.watermarkWarningConfig.visible = true;
        renderWatermark(chart, 'watermarkWarningConfig', 'watermarkWarning', 'chart-watermark-warning');
    }

    ChartWatermark.prototype.initWatermark = function(chart) {
        chart.watermarkErrorConfig = { text: '', visible: false };
        chart.watermarkWarningConfig = { text: '', visible: false };

        chart.updateWatermarkError = function() {
            renderWatermark(chart, 'watermarkErrorConfig', 'watermarkError', 'chart-watermark-error');
        };

        chart.updateWatermarkWarning = function() {
            renderWatermark(chart, 'watermarkWarningConfig', 'watermarkWarning', 'chart-watermark-warning');
        };

        chart.renderError = function(msg) {
            showError(chart, msg);
        };

        chart.renderWarning = function(msg) {
            showWarning(chart, msg);
        };
    };

    ChartWatermark.prototype.refreshWatermark = function(chart) {
        chart.updateWatermarkError();
        chart.updateWatermarkWarning();
    };

    return ChartWatermark;
});
