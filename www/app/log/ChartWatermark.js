define(function() {

    var ICON_SIZE = 20; // px
    var PADDING   = 8;  // px from the top-left corner of the plot

    // ── Icon config ─────────────────────────

    var ICONS = {
        error: {
            faClass:   'fa-solid fa-circle-xmark',
            unicode:   '\uf057',
            color:     '#d9534f'
        },
        warning: {
            faClass:   'fa-solid fa-triangle-exclamation',
            unicode:   '\uf071',
            color:     '#f0ad4e'
        }
    };

    // ── Icon rendering ────────────────────────────────────────────────────────

    function renderIcon(chart, iconCfg, pos) {
        return chart.renderer
            .text(iconCfg.unicode, pos.x, pos.y)
            .css({
                fontFamily: '"Font Awesome 6 Free", "Font Awesome 5 Free"',
                fontWeight:  900,
                fontSize:    ICON_SIZE + 'px',
                color:       iconCfg.color,
                cursor:      'pointer'
            })
            .attr({ zIndex: 5 })
            .add();
    }

    // ── Dynamic position with offset if both icons are visible ────────────────

    function getPosition(chart, elementName) {
        var baseX = chart.plotLeft + PADDING;
        var y     = chart.plotTop  + PADDING;

        var otherName = (elementName === 'watermarkWarning')
            ? 'watermarkError'
            : 'watermarkWarning';

        var offset = (chart[otherName] && chart[otherName].element)
            ? ICON_SIZE + PADDING
            : 0;

        return { x: baseX + offset, y: y };
    }

    // ── Floating native tooltip ───────────────────────────────────────────────

    function attachTooltip(element, message) {
        var node = element.element;
        node.setAttribute('title', message);

        node.addEventListener('mouseenter', function(e) {
            var tip = document.createElement('div');
            tip.id  = 'chart-watermark-panel-tooltip';
            tip.textContent = message;
            Object.assign(tip.style, {
                position:      'fixed',
                background:    '#333',
                color:         '#fff',
                padding:       '6px 10px',
                borderRadius:  '4px',
                fontSize:      '12px',
                maxWidth:      '280px',
                zIndex:        9999,
                pointerEvents: 'none',
                top:  (e.clientY + 12) + 'px',
                left: (e.clientX + 12) + 'px'
            });
            document.body.appendChild(tip);
        });
        node.addEventListener('mousemove', function(e) {
            var tip = document.getElementById('chart-watermark-panel-tooltip');
            if (tip) {
                tip.style.top  = (e.clientY + 12) + 'px';
                tip.style.left = (e.clientX + 12) + 'px';
            }
        });
        node.addEventListener('mouseleave', function() {
            var tip = document.getElementById('chart-watermark-panel-tooltip');
            if (tip) tip.remove();
        });
    }

    // ── Rendering engine ──────────────────────────────────────────────────────

    function renderPanel(chart, configName, elementName, iconKey) {
        var config  = chart[configName];
        var iconCfg = ICONS[iconKey];

        if (!config || !config.visible) {
            if (chart[elementName]) {
                chart[elementName].destroy();
                chart[elementName] = null;
                var tip = document.getElementById('chart-watermark-panel-tooltip');
                if (tip) tip.remove();
            }
            return;
        }

        var pos = getPosition(chart, elementName);

        if (chart[elementName]) {
            chart[elementName].attr({ x: pos.x, y: pos.y });
            return;
        }

        chart[elementName] = renderIcon(chart, iconCfg, pos);
        attachTooltip(chart[elementName], config.text);
    }

    // ── Internal methods API ────────────────────────────────────────────────────────────

    function showPanel(chart, configName, elementName, iconKey, message) {
        chart[configName].text    = message;
        chart[configName].visible = true;
        renderPanel(chart, configName, elementName, iconKey);
    }

    function hidePanel(chart, configName, elementName) {
        chart[configName].visible = false;
        renderPanel(chart, configName, elementName);
    }

    // ── Public API ────────────────────────────────────────────────────────────

    function showError(chart, errorMessage) {
        chart.watermarkErrorConfig.text      = errorMessage;
        chart.watermarkWarningConfig.visible = false;
        renderPanel(chart, 'watermarkWarningConfig', 'watermarkWarning', 'warning');
        chart.watermarkErrorConfig.visible   = true;
        renderPanel(chart, 'watermarkErrorConfig',   'watermarkError',   'error');
    }

    // call with chart.showWarning()
    function showWarning(chart, warningMessage) {
        chart.watermarkWarningConfig.text    = warningMessage;
        chart.watermarkErrorConfig.visible   = false;
        renderPanel(chart, 'watermarkErrorConfig',   'watermarkError',   'error');
        chart.watermarkWarningConfig.visible = true;
        renderPanel(chart, 'watermarkWarningConfig', 'watermarkWarning', 'warning');
    }

    function init(chart) {
        chart.watermarkErrorConfig   = { text: '', visible: false };
        chart.watermarkWarningConfig = { text: '', visible: false };

        chart.updateWatermarkError   = function() {
            renderPanel(chart, 'watermarkErrorConfig',   'watermarkError',   'error');
        };
        chart.updateWatermarkWarning = function() {
            renderPanel(chart, 'watermarkWarningConfig', 'watermarkWarning', 'warning');
        };
        chart.renderError   = function(msg) { showError(chart,   $.t(msg)); };
        chart.renderWarning = function(msg) { showWarning(chart, $.t(msg)); };
    }

    function refresh(chart) {
        chart.updateWatermarkError();
        chart.updateWatermarkWarning();
    }

    return { init, refresh };
});
