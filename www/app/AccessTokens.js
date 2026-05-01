define(['app'], function (app) {

	app.controller('AccessTokensCtrl', ['$scope', '$rootScope', '$http', function ($scope, $rootScope, $http) {

		var oTable;

		var rightsLabel = function (rights) {
			if (rights === 0) return $.t('Viewer');
			if (rights === 1) return $.t('User');
			return $.t('Admin');
		};

		var formatExpiry = function (expiry) {
			if (!expiry || expiry === 0) return $.t('Never');
			return new Date(expiry * 1000).toLocaleDateString();
		};

		var formatLastUpdate = function (lastUpdate) {
			if (!lastUpdate || lastUpdate === '') return '-';
			return lastUpdate;
		};

		$scope.loadTokens = function () {
			$http.get('json.htm?type=command&param=getaccesstokens').then(function (response) {
				var data = response.data;
				if (data.status !== 'OK') {
					bootbox.alert($.t('Problem loading Access Tokens!'));
					return;
				}
				var tokens = data.result || [];

				if ($.fn.dataTable.isDataTable('#accesstokentable')) {
					oTable = $('#accesstokentable').dataTable();
				} else {
					oTable = $('#accesstokentable').dataTable({
						'sDom': '<"H"lfrC>t<"F"ip>',
						'bStateSave': true,
						'bJQueryUI': true,
						'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, $.t('All')]],
						'iDisplayLength': 25,
						'sPaginationType': 'full_numbers',
						language: $.DataTableLanguage
					});
					$('#accesstokentable').off('click', '.btn-danger').on('click', '.btn-danger', function () {
						var idx = $(this).data('idx');
						var name = $(this).data('name');
						DeleteAccessToken(idx, name);
					});
				}

				oTable.fnClearTable();
				tokens.forEach(function (token) {
					oTable.fnAddData([
						token.Name,
						rightsLabel(token.Rights),
						token.CreatedAt || '-',
						formatLastUpdate(token.LastUpdate),
						formatExpiry(token.Expiry),
						'<button class="btn btn-danger btn-xs" data-idx="' + token.idx + '" data-name="' + $('<div/>').text(token.Name).html() + '">' + $.t('Delete') + '</button>'
					]);
				});

				$('#accesstokentable').i18n();
			}, function () {
				bootbox.alert($.t('Problem loading Access Tokens!'));
			});
		};

		$scope.createToken = function () {
			var dlg = bootbox.dialog({
				title: $.t('Create Access Token'),
				message: [
					'<table class="display" border="0" cellpadding="0" cellspacing="20">',
					'<tr>',
					'<td align="right" style="width:110px"><span data-i18n="Name">' + $.t('Name') + '</span>:</td>',
					'<td><input type="text" id="at-name" style="width:200px;padding:.2em;" class="text ui-widget-content ui-corner-all" placeholder="' + $.t('Token name') + '"></td>',
					'</tr>',
					'<tr>',
					'<td align="right" style="width:110px"><span data-i18n="Rights">' + $.t('Rights') + '</span>:</td>',
					'<td><select id="at-rights" class="combobox ui-corner-all" style="width:110px">',
					'<option value="0">' + $.t('Viewer') + '</option>',
					'<option value="1">' + $.t('User') + '</option>',
					'<option value="2">' + $.t('Admin') + '</option>',
					'</select></td>',
					'</tr>',
					'<tr>',
					'<td align="right" style="width:110px"><span data-i18n="Expiry">' + $.t('Expiry') + '</span>:</td>',
					'<td><select id="at-expiry" class="combobox ui-corner-all" style="width:110px">',
					'<option value="0">' + $.t('Never') + '</option>',
					'<option value="30">30 ' + $.t('days') + '</option>',
					'<option value="90">90 ' + $.t('days') + '</option>',
					'<option value="365">1 ' + $.t('year') + '</option>',
					'</select></td>',
					'</tr>',
					'</table>'
				].join(''),
				buttons: {
					cancel: {
						label: $.t('Cancel'),
						className: 'btn-default'
					},
					confirm: {
						label: $.t('Create'),
						className: 'btn-success',
						callback: function () {
							var name = $('#at-name').val().trim();
							var rights = $('#at-rights').val();
							var expiry = $('#at-expiry').val();

							if (name === '') {
								bootbox.alert($.t('Please enter a token name!'));
								return false;
							}

							$http.get('json.htm?type=command&param=createaccesstoken&name=' + encodeURIComponent(name) + '&rights=' + rights + '&expiry=' + expiry).then(function (response) {
								var data = response.data;
								if (data.status !== 'OK') {
									bootbox.alert($.t('Problem creating Access Token!') + (data.statustext ? '<br>' + data.statustext : ''));
									return;
								}
								showTokenOnce(data.token);
								$scope.loadTokens();
							}, function () {
								bootbox.alert($.t('Problem creating Access Token!'));
							});
						}
					}
				}
			});
			dlg.on('shown.bs.modal', function () {
				$('#at-name').trigger('focus');
			});
		};

		DeleteAccessToken = function (idx, name) {
			bootbox.dialog({
				title: $.t('Delete Access Token'),
				message: $.t('Are you sure you want to delete the token "') + name + '"?',
				buttons: {
					cancel: {
						label: $.t('Cancel'),
						className: 'btn-default'
					},
					confirm: {
						label: $.t('Delete'),
						className: 'btn-danger',
						callback: function () {
							$http.get('json.htm?type=command&param=deleteaccesstoken&idx=' + idx).then(function (response) {
								var data = response.data;
								if (data.status !== 'OK') {
									bootbox.alert($.t('Problem deleting Access Token!'));
									return;
								}
								$scope.loadTokens();
							}, function () {
								bootbox.alert($.t('Problem deleting Access Token!'));
							});
						}
					}
				}
			});
		};

		var showTokenOnce = function (token) {
			var message = [
				'<div class="alert alert-warning"><strong>' + $.t('Important') + ':</strong> ' + $.t('This token will NOT be shown again. Copy it now and store it securely.') + '</div>',
				'<div class="form-group">',
				'<label>' + $.t('Your Access Token') + '</label>',
				'<textarea id="at-token-display" readonly rows="6" style="width:100%;resize:none;word-break:break-all;cursor:text;color:var(--dz-input-text);background-color:var(--dz-input-bg);border:1px solid var(--dz-input-border);padding:6px;font-family:monospace;font-size:12px;">' + token + '</textarea>',
				'<button class="btn btn-default" id="at-copy-btn" type="button" style="margin-top:6px;">' + $.t('Copy') + '</button>',
				'</div>'
			].join('');

			var dialog = bootbox.dialog({
				title: $.t('Access Token Created'),
				message: message,
				size: 'large',
				buttons: {
					ok: {
						label: $.t('Done'),
						className: 'btn-primary'
					}
				},
				onEscape: true
			});

			dialog.on('shown.bs.modal', function () {
				$('#at-copy-btn').on('click', function () {
					var input = document.getElementById('at-token-display');
					input.select();
					if (navigator.clipboard && navigator.clipboard.writeText) {
						navigator.clipboard.writeText(token).then(function () {
							$('#at-copy-btn').text($.t('Copied!'));
						});
					} else {
						document.execCommand('copy');
						$('#at-copy-btn').text($.t('Copied!'));
					}
				});
			});
		};

		function init() {
			$scope.MakeGlobalConfig();
			$scope.loadTokens();
		}

		init();
	}]);
});
