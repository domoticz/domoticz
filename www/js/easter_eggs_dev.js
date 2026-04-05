// Developer Easter Egg Test Panel — NOT for production.
//
// To enable: run this once in the browser console, then reload the page:
//   localStorage.setItem('dz_easter_eggs_dev', '1');
//
// To disable:
//   localStorage.removeItem('dz_easter_eggs_dev');

(function () {
    'use strict';

    function waitForEE(cb) {
        if (window.dzEasterEggs) { cb(); return; }
        var attempts = 0;
        var t = setInterval(function () {
            if (window.dzEasterEggs) {
                clearInterval(t);
                cb();
            } else if (++attempts >= 50) {
                clearInterval(t);
            }
        }, 200);
    }

    function buildPanel() {
        var eggs = [
            { id: 'easter',    label: '🥚 Easter' },
            { id: 'halloween', label: '👻 Halloween' },
            { id: 'friday13',  label: '🔦 Friday 13th' },
            { id: 'christmas', label: '❄️ Christmas' },
            { id: 'newyear',   label: '🎉 New Year' },
            { id: 'nightowl',  label: '🌙 Night Owl' },
            { id: 'konami',    label: '🕹️ Konami Code' },
            { id: 'version',   label: '🔮 Version Click' },
            { id: 'console',   label: '👀 Console Msg' },
            { id: 'panic',     label: '✅ Everything is Fine' }
        ];

        var style = document.createElement('style');
        style.textContent =
            '#dz-dev-panel{position:fixed;top:60px;right:16px;z-index:99999;background:#1a1a2e;' +
            'border:2px solid #6a4c93;border-radius:10px;padding:12px;min-width:200px;' +
            'box-shadow:0 4px 24px rgba(0,0,0,.6);font-family:monospace;cursor:move;}' +
            '#dz-dev-panel h4{color:#c084fc;margin:0 0 8px;font-size:.9rem;text-align:center;}' +
            '#dz-dev-panel button{display:block;width:100%;margin:3px 0;padding:5px 10px;' +
            'background:#312e55;color:#e0e0ff;border:1px solid #6a4c93;border-radius:5px;' +
            'cursor:pointer;font-size:.82rem;text-align:left;}' +
            '#dz-dev-panel button:hover{background:#6a4c93;}' +
            '#dz-dev-panel-close{float:right;background:none!important;border:none!important;' +
            'color:#888;font-size:1rem;cursor:pointer;padding:0!important;width:auto!important;}';
        document.head.appendChild(style);

        var panel = document.createElement('div');
        panel.id = 'dz-dev-panel';
        panel.innerHTML = '<h4>\uD83D\uDC23 Easter Egg Dev Panel' +
            '<button id="dz-dev-panel-close">\u2715</button></h4>';

        eggs.forEach(function (egg) {
            var btn = document.createElement('button');
            btn.textContent = egg.label;
            btn.addEventListener('click', function () {
                window.dzEasterEggs.test(egg.id);
            });
            panel.appendChild(btn);
        });

        document.body.appendChild(panel);

        document.getElementById('dz-dev-panel-close').addEventListener('click', function () {
            if (panel.parentNode) panel.parentNode.removeChild(panel);
        });

        var drag = false, ox = 0, oy = 0;
        panel.addEventListener('mousedown', function (e) {
            if (e.target.tagName === 'BUTTON') return;
            drag = true;
            ox = e.clientX - panel.getBoundingClientRect().left;
            oy = e.clientY - panel.getBoundingClientRect().top;
        });
        document.addEventListener('mousemove', function (e) {
            if (!drag) return;
            panel.style.left = (e.clientX - ox) + 'px';
            panel.style.top  = (e.clientY - oy) + 'px';
            panel.style.right = 'auto';
        });
        document.addEventListener('mouseup', function () { drag = false; });
        window.addEventListener('blur', function () { drag = false; });
    }

    waitForEE(buildPanel);
})();
