define(['app'], function (app) {
	'use strict';

	var EE = {
		STORAGE_KEY: 'dz_easter_eggs',

		isEnabled: function () {
			try { return localStorage.getItem('dz_easter_eggs') !== 'false'; } catch (e) { return true; }
		},

		inject: function (id, css) {
			if (document.getElementById(id)) return;
			var s = document.createElement('style');
			s.id = id;
			s.textContent = css;
			document.head.appendChild(s);
		},

		removeEl: function (id) {
			var el = document.getElementById(id);
			if (el && el.parentNode) el.parentNode.removeChild(el);
		},

		wasTriggeredToday: function (name) {
			try { return localStorage.getItem('dz_ee_' + name + '_last') === new Date().toDateString(); } catch (e) { return false; }
		},

		markTriggeredToday: function (name) {
			try { localStorage.setItem('dz_ee_' + name + '_last', new Date().toDateString()); } catch (e) {}
		},

		maybe: function (p) {
			return Math.random() < p;
		},

		_notify: function (type, message, timeout) {
			if (typeof generate_noty === 'function') {
				generate_noty(type, message, timeout);
			}
		},

		getEasterDate: function (year) {
			var a = year % 19;
			var b = Math.floor(year / 100);
			var c = year % 100;
			var d = Math.floor(b / 4);
			var e = b % 4;
			var f = Math.floor((b + 8) / 25);
			var g = Math.floor((b - f + 1) / 3);
			var h = (19 * a + b - d - g + 15) % 30;
			var i = Math.floor(c / 4);
			var k = c % 4;
			var l = (32 + 2 * e + 2 * i - h - k) % 7;
			var m = Math.floor((a + 11 * h + 22 * l) / 451);
			var month = Math.floor((h + l - 7 * m + 114) / 31) - 1; // 0-indexed
			var day = ((h + l - 7 * m + 114) % 31) + 1;
			return { month: month, day: day };
		},

		checkCalendar: function () {
			var now = new Date();
			var month = now.getMonth();
			var day = now.getDate();
			var dow = now.getDay();
			var hour = now.getHours();
			var year = now.getFullYear();

			var easter = EE.getEasterDate(year);
			if (month === easter.month && day === easter.day) {
				if (!EE.wasTriggeredToday('easter') && EE.maybe(0.5)) {
					EE.markTriggeredToday('easter');
					EE.triggerEaster();
				}
			}

			if (month === 9 && day === 31) {
				if (!EE.wasTriggeredToday('halloween') && EE.maybe(0.5)) {
					EE.markTriggeredToday('halloween');
					EE.triggerHalloween();
				}
			}

			if (dow === 5 && day === 13) {
				if (!EE.wasTriggeredToday('friday13') && EE.maybe(0.5)) {
					EE.markTriggeredToday('friday13');
					EE.triggerFriday13();
				}
			}

			if (month === 11 && day === 25) {
				if (!EE.wasTriggeredToday('christmas') && EE.maybe(0.5)) {
					EE.markTriggeredToday('christmas');
					EE.triggerChristmas();
				}
			}

			if (month === 0 && day === 1) {
				if (!EE.wasTriggeredToday('newyear') && EE.maybe(0.5)) {
					EE.markTriggeredToday('newyear');
					EE.triggerNewYear();
				}
			}

			if (hour >= 2 && hour < 4) {
				if (!EE.wasTriggeredToday('nightowl') && EE.maybe(0.3)) {
					EE.markTriggeredToday('nightowl');
					EE.triggerNightOwl();
				}
			}
		},

		triggerEaster: function () {
			EE.inject('dz-easter-css', [
				'@keyframes dz-egg-float {',
				'  0%   { transform: translateY(0) rotate(0deg); opacity: 1; }',
				'  100% { transform: translateY(-120vh) rotate(360deg); opacity: 0; }',
				'}',
				'.dz-egg {',
				'  position: fixed;',
				'  bottom: -60px;',
				'  font-size: 2rem;',
				'  animation: dz-egg-float 8s ease-in forwards;',
				'  pointer-events: none;',
				'  z-index: 99999;',
				'}'
			].join(''));

			var emojis = ['🥚', '🐣', '🐰', '🌸', '🌷', '🥚', '🐣', '🐰', '🌸', '🌷', '🥚', '🐣'];
			for (var i = 0; i < 12; i++) {
				(function (idx) {
					setTimeout(function () {
						var span = document.createElement('span');
						span.className = 'dz-egg';
						span.textContent = emojis[idx];
						span.style.left = (Math.random() * 90 + 5) + 'vw';
						document.body.appendChild(span);
						setTimeout(function () {
							if (span.parentNode) span.parentNode.removeChild(span);
						}, 12000);
					}, idx * 300);
				})(i);
			}

			setTimeout(function () { EE._notify('success', '🥚 Happy Easter!', 4000); }, 800);
		},

		triggerHalloween: function () {
			EE.inject('dz-halloween-css', [
				'#dz-ghost {',
				'  position: fixed;',
				'  bottom: 20px;',
				'  right: 20px;',
				'  font-size: 4rem;',
				'  z-index: 99999;',
				'  pointer-events: none;',
				'  animation: dz-ghost-in 1s ease forwards;',
				'}',
				'#dz-ghost.leaving {',
				'  animation: dz-ghost-out 1.5s ease forwards;',
				'}',
				'@keyframes dz-ghost-in {',
				'  from { opacity: 0; transform: translateY(20px); }',
				'  to   { opacity: 1; transform: translateY(0); }',
				'}',
				'@keyframes dz-ghost-out {',
				'  from { opacity: 1; transform: translateY(0); }',
				'  to   { opacity: 0; transform: translateY(-20px); }',
				'}'
			].join(''));

			var ghost = document.createElement('div');
			ghost.id = 'dz-ghost';
			ghost.textContent = '👻';
			document.body.appendChild(ghost);

			setTimeout(function () { ghost.classList.add('leaving'); }, 5000);
			setTimeout(function () { EE.removeEl('dz-ghost'); }, 6500);

			setTimeout(function () { EE._notify('warning', '👻 Something lurks in the dark…', 3500); }, 1200);
		},

		triggerFriday13: function () {
			EE.inject('dz-friday13-css', [
				'@keyframes dz-flicker {',
				'  0%,100% { opacity: 0; }',
				'  10%,30%,50%,70% { opacity: 0.85; }',
				'  20%,40%,60%,80% { opacity: 0; }',
				'}',
				'#dz-flicker-overlay {',
				'  position: fixed;',
				'  inset: 0;',
				'  background: #000;',
				'  z-index: 99998;',
				'  pointer-events: none;',
				'  animation: dz-flicker 1s ease forwards;',
				'}'
			].join(''));

			var overlay = document.createElement('div');
			overlay.id = 'dz-flicker-overlay';
			document.body.appendChild(overlay);
			setTimeout(function () { EE.removeEl('dz-flicker-overlay'); }, 1300);

			setTimeout(function () { EE._notify('error', '🔦 Friday the 13th… sleep tight.', 4000); }, 1100);
		},

		triggerChristmas: function () {
			EE.inject('dz-christmas-css', [
				'@keyframes dz-snow-fall {',
				'  0%   { transform: translateY(-10vh) rotate(0deg); opacity: 1; }',
				'  100% { transform: translateY(110vh) rotate(360deg); opacity: 0.3; }',
				'}',
				'.dz-flake {',
				'  position: fixed;',
				'  top: -40px;',
				'  font-size: 1.5rem;',
				'  pointer-events: none;',
				'  z-index: 99999;',
				'  animation: dz-snow-fall linear forwards;',
				'}'
			].join(''));

			var flakes = ['❄️', '❅', '❆', '⛄'];
			for (var i = 0; i < 20; i++) {
				(function (idx) {
					setTimeout(function () {
						var span = document.createElement('span');
						span.className = 'dz-flake';
						span.textContent = flakes[idx % flakes.length];
						span.style.left = (Math.random() * 95 + 2) + 'vw';
						span.style.animationDuration = (5 + Math.random() * 5) + 's';
						document.body.appendChild(span);
						setTimeout(function () {
							if (span.parentNode) span.parentNode.removeChild(span);
						}, 18000);
					}, idx * 400);
				})(i);
			}

			setTimeout(function () { EE._notify('success', '🎄 Merry Christmas! Ho ho ho!', 4000); }, 600);
		},

		triggerNewYear: function () {
			EE.inject('dz-newyear-css', [
				'@keyframes dz-conf-fall {',
				'  0%   { transform: translateY(-10vh) rotate(0deg); opacity: 1; }',
				'  100% { transform: translateY(110vh) rotate(720deg); opacity: 0; }',
				'}',
				'.dz-conf {',
				'  position: fixed;',
				'  top: -20px;',
				'  width: 8px;',
				'  height: 14px;',
				'  pointer-events: none;',
				'  z-index: 99999;',
				'  animation: dz-conf-fall linear forwards;',
				'}'
			].join(''));

			var colors = ['#f43f5e', '#f97316', '#eab308', '#22c55e', '#3b82f6', '#a855f7'];
			for (var i = 0; i < 40; i++) {
				(function (idx) {
					setTimeout(function () {
						var div = document.createElement('div');
						div.className = 'dz-conf';
						div.style.left = (Math.random() * 98 + 1) + 'vw';
						div.style.backgroundColor = colors[idx % colors.length];
						div.style.animationDuration = (4 + Math.random() * 4) + 's';
						document.body.appendChild(div);
						setTimeout(function () {
							if (div.parentNode) div.parentNode.removeChild(div);
						}, 12000);
					}, idx * 100);
				})(i);
			}

			setTimeout(function () { EE._notify('success', '🎉 Happy New Year! 🥂', 5000); }, 500);
		},

		triggerNightOwl: function () {
			EE.inject('dz-nightowl-css', [
				'@keyframes dz-nightowl-fade {',
				'  0%   { opacity: 0; }',
				'  15%  { opacity: 0.07; }',
				'  85%  { opacity: 0.07; }',
				'  100% { opacity: 0; }',
				'}',
				'#dz-nightowl {',
				'  position: fixed;',
				'  inset: 0;',
				'  background: #f59e0b;',
				'  pointer-events: none;',
				'  z-index: 99997;',
				'  animation: dz-nightowl-fade 6s ease forwards;',
				'}'
			].join(''));

			var overlay = document.createElement('div');
			overlay.id = 'dz-nightowl';
			document.body.appendChild(overlay);
			setTimeout(function () { EE.removeEl('dz-nightowl'); }, 6200);

			EE._notify('info', '🌙 You\'re up late… everything looks calm.', 5000);
		},

		setupKonami: function () {
			var seq = [38,38,40,40,37,39,37,39,66,65];
			var pos = 0;
			EE._konamiListener = function (e) {
				if (e.keyCode === seq[pos]) {
					pos++;
					if (pos === seq.length) {
						pos = 0;
						EE._triggerKonamiEffect();
					}
				} else {
					pos = (e.keyCode === seq[0]) ? 1 : 0;
				}
			};
			document.addEventListener('keydown', EE._konamiListener);
		},

		_triggerKonamiEffect: function () {
			EE.inject('dz-konami-css', [
				'@keyframes dz-glitch {',
				'  0%,100% { filter: none; }',
				'  20%     { filter: invert(1) hue-rotate(90deg) blur(1px); }',
				'  40%     { filter: invert(0) hue-rotate(180deg); }',
				'  60%     { filter: invert(1) hue-rotate(270deg) blur(2px); }',
				'  80%     { filter: hue-rotate(360deg) blur(1px); }',
				'}',
				'.dz-konami-active {',
				'  animation: dz-glitch 1.3s steps(1) forwards;',
				'}'
			].join(''));

			document.body.classList.add('dz-konami-active');
			setTimeout(function () { document.body.classList.remove('dz-konami-active'); }, 1300);

			EE._notify('info', '🕹️ KONAMI! +30 lives granted. You\'re welcome.', 4000);
		},

		setupVersionClick: function () {
			var clicks = 0, timer = null;
			EE._versionClickHandler = function () {
				clicks++;
				clearTimeout(timer);
				timer = setTimeout(function () { clicks = 0; }, 5000);
				if (clicks >= 5) {
					clicks = 0;
					clearTimeout(timer);
					EE._triggerVersionDialog();
				}
			};
			$(document).on('click', '#appversion, .version-badge', EE._versionClickHandler);
		},

		_triggerVersionDialog: function () {
			EE.inject('dz-vdlg-css', [
				'#dz-vdlg-bg {',
				'  position: fixed;',
				'  inset: 0;',
				'  background: rgba(0,0,0,0.6);',
				'  z-index: 100000;',
				'  display: flex;',
				'  align-items: center;',
				'  justify-content: center;',
				'}',
				'#dz-vdlg {',
				'  background: var(--dz-modal-bg);',
				'  color: var(--dz-modal-text);',
				'  border-radius: 12px;',
				'  padding: 2rem 2.5rem;',
				'  max-width: 380px;',
				'  width: 90%;',
				'  box-shadow: 0 8px 40px rgba(0,0,0,0.6);',
				'  text-align: center;',
				'}',
				'#dz-vdlg h2 {',
				'  font-size: 1.6rem;',
				'  margin-bottom: 1rem;',
				'  color: var(--dz-widget-accent);',
				'}',
				'#dz-vdlg ul {',
				'  list-style: none;',
				'  padding: 0;',
				'  margin: 0 0 1.5rem;',
				'  text-align: left;',
				'  font-size: 0.9rem;',
				'  line-height: 1.9;',
				'}',
				'#dz-vdlg strong {',
				'  color: var(--dz-widget-accent);',
				'}',
				'#dz-vdlg-close {',
				'  background: var(--dz-btn-primary-bg);',
				'  color: var(--dz-btn-primary-text);',
				'  border: none;',
				'  border-radius: 6px;',
				'  padding: 0.5rem 1.5rem;',
				'  cursor: pointer;',
				'  font-size: 0.95rem;',
				'}'
			].join(''));

			var bg = document.createElement('div');
			bg.id = 'dz-vdlg-bg';

			var dlg = document.createElement('div');
			dlg.id = 'dz-vdlg';
			dlg.innerHTML = [
				'<h2>🔮 You found it.</h2>',
				'<ul>',
				'<li>📦 Lines of C++: <strong>too many</strong></li>',
				'<li>🐛 Bugs fixed: <strong>most of them</strong></li>',
				'<li>😴 Hours of sleep: <strong>negotiable</strong></li>',
				'</ul>',
				'<button id="dz-vdlg-close">Close</button>'
			].join('');

			bg.appendChild(dlg);
			document.body.appendChild(bg);

			function closeDlg() { EE.removeEl('dz-vdlg-bg'); }

			document.getElementById('dz-vdlg-close').addEventListener('click', closeDlg);
			bg.addEventListener('click', function (e) { if (e.target === bg) closeDlg(); });
		},

		setupEverythingFine: function () {
			EE._everythingFineHandler = function (e) {
				if ($(e.target).closest('#appversion').length) return;
				EE._triggerPanic();
			};
			$(document).on('dblclick', 'a.brand', EE._everythingFineHandler);
		},

		destroy: function () {
			if (EE._konamiListener) {
				document.removeEventListener('keydown', EE._konamiListener);
				EE._konamiListener = null;
			}
			if (EE._versionClickHandler) {
				$(document).off('click', '#appversion, .version-badge', EE._versionClickHandler);
				EE._versionClickHandler = null;
			}
			if (EE._everythingFineHandler) {
				$(document).off('dblclick', 'a.brand', EE._everythingFineHandler);
				EE._everythingFineHandler = null;
			}
		},

		_triggerPanic: function () {
			if (document.getElementById('dz-panic')) return;

			EE.inject('dz-panic-css', [
				'@keyframes dz-panic-in {',
				'  from { transform: translateY(-100%); }',
				'  to   { transform: translateY(0); }',
				'}',
				'@keyframes dz-panic-out {',
				'  from { opacity: 1; }',
				'  to   { opacity: 0; }',
				'}',
				'#dz-panic {',
				'  position: fixed;',
				'  top: 0;',
				'  left: 0;',
				'  right: 0;',
				'  background: linear-gradient(90deg, #16a34a, #15803d);',
				'  color: #fff;',
				'  text-align: center;',
				'  padding: 0.75rem 1rem;',
				'  font-size: 0.95rem;',
				'  font-weight: bold;',
				'  z-index: 100001;',
				'  animation: dz-panic-in 0.4s ease forwards;',
				'}',
				'#dz-panic.fading {',
				'  animation: dz-panic-out 0.6s ease forwards;',
				'}'
			].join(''));

			var banner = document.createElement('div');
			banner.id = 'dz-panic';
			banner.textContent = '✅ System Status: EVERYTHING IS FINE — No issues detected. All devices nominal. Humans calm.';
			document.body.appendChild(banner);

			setTimeout(function () { banner.classList.add('fading'); }, 5000);
			setTimeout(function () { EE.removeEl('dz-panic'); }, 5600);
		},

		consoleMessage: function () {
			console.log('%c👀 You found the console!', 'color:#8b5cf6;font-size:1.2rem;font-weight:bold;');
			console.log('%cThere are no secrets here. Or are there? 🏠', 'color:#a0a0c0;font-size:.9rem;');
			console.log('%cDomoticz — keeping your home smart since 2012.', 'color:#6b7280;font-size:.8rem;');
		},

		test: function (name) {
			var map = {
				easter:    EE.triggerEaster,
				halloween: EE.triggerHalloween,
				friday13:  EE.triggerFriday13,
				christmas: EE.triggerChristmas,
				newyear:   EE.triggerNewYear,
				nightowl:  EE.triggerNightOwl,
				konami:    EE._triggerKonamiEffect,
				version:   EE._triggerVersionDialog,
				console:   EE.consoleMessage,
				panic:     EE._triggerPanic
			};
			if (map[name]) map[name]();
		},

		init: function () {
			if (!EE.isEnabled()) return;
			EE.consoleMessage();
			EE.setupKonami();
			EE.setupVersionClick();
			EE.setupEverythingFine();
			setTimeout(EE.checkCalendar, 1500);
			try {
				if (localStorage.getItem('dz_easter_eggs_dev') === '1') {
					var s = document.createElement('script');
					s.src = 'js/easter_eggs_dev.js';
					document.head.appendChild(s);
				}
			} catch (e) {}
		}
	};

	window.dzEasterEggs = EE;
	return EE;
});
