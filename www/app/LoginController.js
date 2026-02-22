define(['app'], function (app) {
	app.controller('LoginController', ['permissions', '$scope', '$rootScope', '$location', '$http', '$interval', '$window', 'md5', function (permissions, $scope, $rootScope, $location, $http, $interval, $window, md5) {

		$scope.failcounter = 0;
		$scope.authView = 'login';
		$scope.authMethod = 'password';
		$scope.passkeyAvailable = false;

		// --- Auth method helpers ---

		$scope.setAuthMethod = function (method) {
			$scope.authMethod = method;
		};

		$scope.backToLogin = function () {
			$scope.authView = 'login';
			$('#totp').val('');
		};

		function shake2faCard() {
			var card = document.getElementById('2fa-card');
			if (!card) { return; }
			card.classList.remove('shake');
			// Force reflow so the animation restarts if already applied
			void card.offsetWidth;
			card.classList.add('shake');
			card.addEventListener('animationend', function handler() {
				card.classList.remove('shake');
				card.removeEventListener('animationend', handler);
			});
		}

		$scope.watchTotp = function () {
			var val = $('#totp').val().replace(/\D/g, '');
			// Keep only digits in the field
			$('#totp').val(val);
			if (val.length === 6) {
				$scope.DoMfaLogin();
			}
		};

		// --- Shared login result handlers ---

		function handleLoginSuccess(data) {
			var permissionList = {
				isloggedin: false,
				rights: -1,
				user: '',
				canlogout: true
			};
			if (data.user != "") {
				permissionList.isloggedin = true;
				permissionList.user = data.user;
				permissionList.rights = parseInt(data.rights);
				permissions.setPermissions(permissionList);

				$rootScope.GetGlobalConfig();

				$location.path('/Dashboard');

				// Show security recommendation only after initial setup wizard completion
				if (parseInt(data.rights) === 2 && sessionStorage.getItem('setupJustCompleted')) {
					sessionStorage.removeItem('setupJustCompleted');
					setTimeout(function() {
						ShowNotify($.t('Consider enabling 2FA or passkeys for your admin account in User Settings'), 8000);
					}, 2000);
				}
			}
		}

		function handleLoginFailure(message) {
			HideNotify();
			$scope.failcounter += 1;
			if ($scope.failcounter > 3) {
				window.location.href = "https://hmpg.net/";
				return;
			}
			ShowNotify($.t(message), 2500, true);
		}

		// --- Login actions ---

		$scope.DoLogin = function () {
			var musername = encodeURIComponent(btoa($('#username').val()));
			var mpassword = encodeURIComponent(md5.createHash($('#password').val()));
			var bRememberMe = $('#rememberme').is(":checked");

			var fd = new FormData();
			fd.append('username', musername);
			fd.append('password', mpassword);
			fd.append('rememberme', bRememberMe);
			$http.post('json.htm?type=command&param=logincheck', fd, {
				transformRequest: angular.identity,
				headers: { 'Content-Type': undefined }
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.require2fa != "undefined" && data.require2fa == "true") {
					$scope.authView = '2fa';
					// Focus TOTP input after Angular has rendered the 2FA view
					setTimeout(function () {
						var totpEl = document.getElementById('totp');
						if (totpEl) { totpEl.focus(); }
					}, 50);
					return;
				}
				if (data.status != "OK") {
					handleLoginFailure('Incorrect Username/Password!');
					return;
				}
				handleLoginSuccess(data);
			}, function errorCallback(response) {
				handleLoginFailure('Incorrect Username/Password!');
			});
		};

		$scope.DoMfaLogin = function () {
			var musername = encodeURIComponent(btoa($('#username').val()));
			var mpassword = encodeURIComponent(md5.createHash($('#password').val()));
			var bRememberMe = $('#rememberme').is(":checked");

			var fd = new FormData();
			fd.append('username', musername);
			fd.append('password', mpassword);
			fd.append('rememberme', bRememberMe);
			fd.append('2fatotp', $('#totp').val());
			$http.post('json.htm?type=command&param=logincheck', fd, {
				transformRequest: angular.identity,
				headers: { 'Content-Type': undefined }
			}).then(function successCallback(response) {
				var data = response.data;
				if (data.status != "OK") {
					handleLoginFailure('Incorrect 2FA Code!');
					shake2faCard();
					$('#totp').val('');
					return;
				}
				handleLoginSuccess(data);
			}, function errorCallback(response) {
				handleLoginFailure('Incorrect 2FA Code!');
				shake2faCard();
				$('#totp').val('');
			});
		};

		$scope.DoPasskeyLogin = function () {
			if (!window.PublicKeyCredential) {
				ShowNotify($.t('Your browser does not support passkeys'), 3000, true);
				return;
			}

			// Step 1: Get authentication options from server
			$http.get('json.htm?type=command&param=passkeylogin-begin').then(function (response) {
				var data = response.data;
				if (data.status !== 'OK') {
					ShowNotify($.t('Failed to start passkey authentication'), 3000, true);
					return;
				}

				// Step 2: Build PublicKeyCredentialRequestOptions
				var getOptions = {
					publicKey: {
						challenge: base64urlToBuffer(data.challenge),
						timeout: data.timeout,
						rpId: data.rpId,
						userVerification: data.userVerification,
						allowCredentials: (data.allowCredentials || []).map(function (cred) {
							return {
								type: cred.type,
								id: base64urlToBuffer(cred.id)
							};
						})
					}
				};

				// Step 3: Call WebAuthn API (native Promise - must nest to avoid
				// breaking Angular's digest cycle with flat $q chain)
				navigator.credentials.get(getOptions).then(function (assertion) {
					// Step 4: Send assertion to server
					var bRememberMe = $('#rememberme').is(':checked');

					var fd = new FormData();
					fd.append('credentialId', bufferToBase64url(assertion.rawId));
					fd.append('authenticatorData', bufferToBase64url(assertion.response.authenticatorData));
					fd.append('clientDataJSON', bufferToBase64url(assertion.response.clientDataJSON));
					fd.append('signature', bufferToBase64url(assertion.response.signature));
					fd.append('rememberme', bRememberMe);

					$http.post('json.htm?type=command&param=passkeylogin-complete', fd, {
						transformRequest: angular.identity,
						headers: { 'Content-Type': undefined }
					}).then(function (response) {
						var data = response.data;
						if (data.status !== 'OK') {
							handleLoginFailure('Passkey authentication failed');
							return;
						}
						handleLoginSuccess(data);
					}, function () {
						handleLoginFailure('Passkey authentication failed');
					});
				}).catch(function (err) {
					console.error('Passkey authentication error:', err);
					if (err.name === 'NotAllowedError') {
						// User cancelled - don't show error
					} else {
						$scope.$apply(function () {
							ShowNotify($.t('Passkey authentication failed: ') + err.message, 3000, true);
						});
					}
				});
			});
		};

		// --- Canvas particle constellation animation ---

		function initCanvas() {
			var canvas = document.getElementById('canvas-login');
			if (!canvas) { return; }
			var ctx = canvas.getContext('2d');
			var particles = [];
			var particleCount = 80;
			var connectionDistance = 120;
			var mouse = { x: null, y: null, radius: 150 };

			// Set canvas size
			function resizeCanvas() {
				canvas.width = window.innerWidth;
				canvas.height = window.innerHeight;
			}
			resizeCanvas();
			window.addEventListener('resize', resizeCanvas);

			// Track mouse
			window.addEventListener('mousemove', function (e) {
				mouse.x = e.x;
				mouse.y = e.y;
			});
			window.addEventListener('mouseout', function () {
				mouse.x = null;
				mouse.y = null;
			});

			// Particle class
			function Particle() {
				this.x = Math.random() * canvas.width;
				this.y = Math.random() * canvas.height;
				this.vx = (Math.random() - 0.5) * 0.8;
				this.vy = (Math.random() - 0.5) * 0.8;
				this.radius = Math.random() * 2 + 1;
				this.color = 'rgba(100, 200, 255, ' + (Math.random() * 0.5 + 0.5) + ')';
			}

			Particle.prototype.draw = function () {
				ctx.beginPath();
				ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
				ctx.fillStyle = this.color;
				ctx.fill();

				// Glow effect
				ctx.shadowBlur = 15;
				ctx.shadowColor = 'rgba(100, 200, 255, 0.5)';
			};

			Particle.prototype.update = function () {
				// Mouse interaction - particles move away from cursor
				if (mouse.x !== null && mouse.y !== null) {
					var dx = this.x - mouse.x;
					var dy = this.y - mouse.y;
					var dist = Math.sqrt(dx * dx + dy * dy);
					if (dist < mouse.radius) {
						var force = (mouse.radius - dist) / mouse.radius;
						this.vx += (dx / dist) * force * 0.5;
						this.vy += (dy / dist) * force * 0.5;
					}
				}

				// Apply velocity with damping
				this.vx *= 0.99;
				this.vy *= 0.99;

				// Ensure minimum movement
				var speed = Math.sqrt(this.vx * this.vx + this.vy * this.vy);
				if (speed < 0.2) {
					this.vx += (Math.random() - 0.5) * 0.1;
					this.vy += (Math.random() - 0.5) * 0.1;
				}

				this.x += this.vx;
				this.y += this.vy;

				// Wrap around edges
				if (this.x < 0) { this.x = canvas.width; }
				if (this.x > canvas.width) { this.x = 0; }
				if (this.y < 0) { this.y = canvas.height; }
				if (this.y > canvas.height) { this.y = 0; }
			};

			// Create particles
			for (var i = 0; i < particleCount; i++) {
				particles.push(new Particle());
			}

			// Draw connections between nearby particles
			function drawConnections() {
				for (var i = 0; i < particles.length; i++) {
					for (var j = i + 1; j < particles.length; j++) {
						var dx = particles[i].x - particles[j].x;
						var dy = particles[i].y - particles[j].y;
						var dist = Math.sqrt(dx * dx + dy * dy);

						if (dist < connectionDistance) {
							var opacity = 1 - (dist / connectionDistance);
							ctx.beginPath();
							ctx.moveTo(particles[i].x, particles[i].y);
							ctx.lineTo(particles[j].x, particles[j].y);
							ctx.strokeStyle = 'rgba(100, 200, 255, ' + (opacity * 0.4) + ')';
							ctx.lineWidth = 1;
							ctx.stroke();
						}
					}
				}
			}

			// Animation loop
			function animate() {
				ctx.clearRect(0, 0, canvas.width, canvas.height);

				// Draw gradient background
				var gradient = ctx.createRadialGradient(
					canvas.width / 2, canvas.height / 2, 0,
					canvas.width / 2, canvas.height / 2, canvas.width * 0.7
				);
				gradient.addColorStop(0, '#1a1a2e');
				gradient.addColorStop(0.5, '#16213e');
				gradient.addColorStop(1, '#0f0f23');
				ctx.fillStyle = gradient;
				ctx.fillRect(0, 0, canvas.width, canvas.height);

				ctx.shadowBlur = 0;
				drawConnections();

				for (var i = 0; i < particles.length; i++) {
					particles[i].update();
					particles[i].draw();
				}

				requestAnimationFrame(animate);
			}

			animate();
		}

		// --- Base64url helpers for WebAuthn ---

		function base64urlToBuffer(base64url) {
			var base64 = base64url.replace(/-/g, '+').replace(/_/g, '/');
			var padding = base64.length % 4;
			if (padding) base64 += '='.repeat(4 - padding);
			var binary = atob(base64);
			var bytes = new Uint8Array(binary.length);
			for (var i = 0; i < binary.length; i++) {
				bytes[i] = binary.charCodeAt(i);
			}
			return bytes.buffer;
		}

		function bufferToBase64url(buffer) {
			var bytes = new Uint8Array(buffer);
			var binary = '';
			for (var i = 0; i < bytes.length; i++) {
				binary += String.fromCharCode(bytes[i]);
			}
			return btoa(binary).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
		}

		// --- Passkey support detection ---

		function checkPasskeySupport() {
			if (!window.PublicKeyCredential) {
				$scope.passkeyAvailable = false;
				return;
			}
			$http.get('json.htm?type=command&param=haspasskeys').then(function (response) {
				if (response.data.status === 'OK' && response.data.hasPasskeys === true) {
					$scope.passkeyAvailable = true;
				}
			});
		}

		// --- Initialisation ---

		init();

		function init() {
			$.ajax({
				url: "json.htm?type=command&param=getlanguages",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.language != 'undefined') {
						SetLanguage(data.language);
					}
					else {
						SetLanguage('en');
					}
				},
				error: function () {
				}
			});

			var $inputs = $('#username, #password');
			$inputs.each(function () {
				$(this).attr("placeholder", $.t($(this).attr("placeholder")));
			});
			$("#remembermelbl").text($.t("Remember me"));
			$("#username").focus();

			initCanvas();
			checkPasskeySupport();
		}
	}]);
});
