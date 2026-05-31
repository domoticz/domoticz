define(['app'], function (app) {
	app.controller('TipsController', ['$scope', '$http', '$uibModalInstance', function ($scope, $http, $uibModalInstance) {
		var LS_KEY = 'dz_tips_enabled';

		$scope.categories = [];
		$scope.selectedCategory = 'Beginner';
		$scope.allTips = {};
		$scope.tips = [];
		$scope.currentIndex = 0;
		$scope.showOnStartup = true;

		try { $scope.showOnStartup = localStorage.getItem(LS_KEY) !== 'false'; } catch(e) {}

		$http.get('tips.json').then(function(response) {
			var data = response.data;
			$scope.categories = Object.keys(data);
			$scope.allTips = data;
			$scope.selectCategory('Beginner', true);
		});

		$scope.selectCategory = function(cat, randomize) {
			$scope.selectedCategory = cat;
			$scope.tips = $scope.allTips[cat] || [];
			$scope.currentIndex = randomize ? Math.floor(Math.random() * $scope.tips.length) : 0;
		};

		$scope.prev = function() {
			$scope.currentIndex = ($scope.currentIndex - 1 + $scope.tips.length) % $scope.tips.length;
		};

		$scope.next = function() {
			$scope.currentIndex = ($scope.currentIndex + 1) % $scope.tips.length;
		};

		$scope.currentTip = function() {
			return $scope.tips[$scope.currentIndex];
		};

		$scope.toggleStartup = function() {
			try { localStorage.setItem(LS_KEY, String($scope.showOnStartup)); } catch(e) {}
		};

		$scope.close = function() { $uibModalInstance.dismiss(); };
	}]);
});
