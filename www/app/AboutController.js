define(['app'], function (app) {
	app.controller('AboutController', ['$scope', '$rootScope', '$location', '$http', '$interval', '$uibModal', function ($scope, $rootScope, $location, $http, $interval, $uibModal) {

		$scope.strupptime = "-";

		$scope.RefreshUptime = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$http({
				url: "json.htm?type=command&param=getuptime",
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.status != 'undefined') {
					var szUpdate = "";
					if (data.days != 0) {
						szUpdate += data.days + " " + $.t("Days") + ", ";
					}
					if (data.hours != 0) {
						szUpdate += data.hours + " " + $.t("Hours") + ", ";
					}
					if (data.minutes != 0) {
						szUpdate += data.minutes + " " + $.t("Minutes") + ", ";
					}
					szUpdate += data.seconds + " " + $.t("Seconds");
					$scope.strupptime = szUpdate;
					$scope.mytimer = $interval(function () {
						$scope.RefreshUptime();
					}, 5000);
				}
			});
		}

		$scope.openTips = function() {
			require(['TipsController'], function() {
				$uibModal.open({
					templateUrl: 'views/tips.html',
					controller: 'TipsController',
					size: 'md',
					windowClass: 'tips-modal'
				}).result.catch(angular.noop);
			});
		};

		$scope.init = function () {
			$scope.MakeGlobalConfig();
			$scope.RefreshUptime();

			// Particle Constellation Animation
			var canvas = document.getElementById('canvas2');
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
			window.addEventListener('mousemove', function(e) {
				mouse.x = e.x;
				mouse.y = e.y;
			});
			window.addEventListener('mouseout', function() {
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

			Particle.prototype.draw = function() {
				ctx.beginPath();
				ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
				ctx.fillStyle = this.color;
				ctx.fill();

				// Glow effect
				ctx.shadowBlur = 15;
				ctx.shadowColor = 'rgba(100, 200, 255, 0.5)';
			};

			Particle.prototype.update = function() {
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
				if (this.x < 0) this.x = canvas.width;
				if (this.x > canvas.width) this.x = 0;
				if (this.y < 0) this.y = canvas.height;
				if (this.y > canvas.height) this.y = 0;
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
		};

		$("#aboutcontent").i18n();
		$scope.init();

		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		});
	}]);
});
