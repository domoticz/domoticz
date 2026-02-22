define(['app'], function (app) {
	app.controller('SetupWizardController', ['$scope', '$location', '$http', 'md5', function ($scope, $location, $http, md5) {

		$scope.setup = {
			username: 'admin',
			password: '',
			confirmPassword: ''
		};
		$scope.errorMessage = '';
		$scope.isSubmitting = false;

		// --- Canvas particle constellation animation (same as LoginController) ---

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

		// --- Password strength indicator ---

		$scope.passwordStrength = null;

		$scope.checkPasswordStrength = function () {
			var pwd = $scope.setup.password;
			if (!pwd || pwd.length === 0) {
				$scope.passwordStrength = null;
				return;
			}
			var hasUpper = /[A-Z]/.test(pwd);
			var hasLower = /[a-z]/.test(pwd);
			var hasNumber = /[0-9]/.test(pwd);
			var hasSpecial = /[^A-Za-z0-9]/.test(pwd);
			var categories = (hasUpper ? 1 : 0) + (hasLower ? 1 : 0) + (hasNumber ? 1 : 0) + (hasSpecial ? 1 : 0);

			if (pwd.length >= 12 && categories >= 3) {
				$scope.passwordStrength = { level: 'strong', label: 'Strong', percent: 100 };
			} else if (pwd.length >= 8 && categories >= 2) {
				$scope.passwordStrength = { level: 'medium', label: 'Medium', percent: 66 };
			} else {
				$scope.passwordStrength = { level: 'weak', label: 'Weak', percent: 33 };
			}
		};

		// --- Setup form submission ---

		$scope.DoSetup = function () {
			if ($scope.isSubmitting) { return; }
			$scope.isSubmitting = true;
			$scope.errorMessage = '';

			if ($scope.setup.password !== $scope.setup.confirmPassword) {
				$scope.errorMessage = 'Passwords do not match';
				$scope.isSubmitting = false;
				return;
			}

			if ($scope.setup.password.length < 5) {
				$scope.errorMessage = 'Password must be at least 5 characters';
				$scope.isSubmitting = false;
				return;
			}

			if ($scope.setup.password.toLowerCase() === 'domoticz') {
				$scope.errorMessage = 'This password is a known default and cannot be used';
				$scope.isSubmitting = false;
				return;
			}

			var md5Password = md5.createHash($scope.setup.password);

			$http.get('json.htm', {
				params: {
					type: 'command',
					param: 'setupwizardcreateadmin',
					username: $scope.setup.username,
					password: md5Password
				}
			}).then(function (response) {
				var data = response.data;
				if (data.status === 'OK') {
					window.needsSetup = false;
					sessionStorage.setItem('setupJustCompleted', 'true');
					$location.path('/Login');
				} else {
					$scope.errorMessage = data.message || 'Failed to create account';
					$scope.isSubmitting = false;
				}
			}, function () {
				$scope.errorMessage = 'Connection error. Please try again.';
				$scope.isSubmitting = false;
			});
		};

		// --- Initialisation ---

		init();

		function init() {
			// Load language so translated error messages work
			$.ajax({
				url: "json.htm?type=command&param=getlanguages",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.language != 'undefined') {
						SetLanguage(data.language);
					} else {
						SetLanguage('en');
					}
				},
				error: function () {}
			});

			// Translate input placeholders
			var $inputs = $('#username, #password, #confirmPassword');
			$inputs.each(function () {
				$(this).attr("placeholder", $.t($(this).attr("placeholder")));
			});

			// Check if setup is actually required — redirect away if not
			$http.get('json.htm', {
				params: { type: 'command', param: 'getsetuprequired' }
			}).then(function (response) {
				var data = response.data;
				if (!data.SetupRequired) {
					$location.path('/Login');
				}
			}).catch(function () {
				// Network error - allow form to render; backend will reject if setup not needed
				console.warn('Failed to check setup status');
			});

			initCanvas();
		}
	}]);
});
