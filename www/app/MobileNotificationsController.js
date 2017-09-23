define(['app'], function (app) {
	app.controller('MobileNotificationsController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'md5', function ($scope, $rootScope, $location, $http, $interval, md5) {

		DeleteMobile = function (uuid) {
			bootbox.confirm($.t("Are you sure to delete this Device?\n\nThis action can not be undone..."), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deletemobiledevice&uuid=" + uuid,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshMobileTable();
						},
						error: function () {
							HideNotify();
							ShowNotify($.t('Problem deleting User!'), 2500, true);
						}
					});
				}
			});
		}

		GetMobileSettings = function () {
			var csettings = {};

			csettings.bEnabled = $('#mobilecontent #mobileparamstable #enabled').is(":checked");
			csettings.name = $("#mobilecontent #mobileparamstable #name").val();
			if (csettings.name == "") {
				ShowNotify($.t('Please enter a Name!'), 2500, true);
				return;
			}
			return csettings;
		}

		UpdateMobile = function (idx) {
			var csettings = GetMobileSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				url: "json.htm?type=command&param=updatemobiledevice&idx=" + idx +
				"&enabled=" + csettings.bEnabled +
				"&name=" + csettings.name,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshMobileTable();
				},
				error: function () {
					ShowNotify($.t('Problem updating Mobile!'), 2500, true);
				}
			});
		}

		SendMobileTestMessage = function(idx)
		{
		    var subsystem = "gcm";
		    var extraparams = "extradata=midx_"+idx;
		    $.ajax({
		        url: "json.htm?type=command&param=testnotification&subsystem=" + subsystem + "&" + extraparams,
		        async: false,
		        dataType: 'json',
		        success: function (data) {
		            if (data.status != "OK") {
		                HideNotify();
	                    ShowNotify($.t('Problem Sending Notification'), 3000, true);
		                return;
		            }
		            else {
		                HideNotify();
		                ShowNotify($.t('Notification sent!<br>Should arrive at your device soon...'), 3000);
		            }
		        },
		        error: function () {
		            HideNotify();
		            ShowNotify($.t('Problem Sending Notification'), 3000, true);
		        }
		    });
		}

		RefreshMobileTable = function () {
			$('#modal').show();

			$.devIdx = -1;

			$('#updelclr #mobileupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #mobiledelete').attr("class", "btnstyle3-dis");

			var oTable = $('#mobiletable').dataTable();
			oTable.fnClearTable();
			$.ajax({
				url: "json.htm?type=mobiles",
				async: false,
				dataType: 'json',
				success: function (data) {

					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var enabledstr = $.t("No");
							if (item.Enabled == "true") {
								enabledstr = $.t("Yes");
							}
							var lUpdateItem = item.LastUpdate;
							lUpdateItem += '&nbsp;<img src="images/add.png" title="' + $.t('Test') + '" onclick="SendMobileTestMessage(' + item.idx + ');">';
							var addId = oTable.fnAddData({
								"DT_RowId": item.idx,
								"Enabled": item.Enabled,
								"Name": item.Name,
								"UUID": item.UUID,
								"0": item.idx,
								"1": enabledstr,
								"2": item.Name,
								"3": item.UUID,
								"4": item.DeviceType,
								"5": lUpdateItem
							});
						});
					}
				}
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#mobiletable tbody").off();
			$("#mobiletable tbody").on('click', 'tr', function () {
				$.devIdx = -1;

				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$('#updelclr #mobileupdate').attr("class", "btnstyle3-dis");
					$('#updelclr #mobiledelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#mobiletable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#updelclr #mobileupdate').attr("class", "btnstyle3");
					$('#updelclr #mobiledelete').attr("class", "btnstyle3");
					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						$.devIdx = idx;
						$.devUUID = data["UUID"];
						$("#updelclr #mobileupdate").attr("href", "javascript:UpdateMobile(" + idx + ")");
						$("#updelclr #mobiledelete").attr("href", "javascript:DeleteMobile('" + $.devUUID + "')");
						$('#mobilecontent #mobileparamstable #enabled').prop('checked', (data["Enabled"] == "true"));
						$("#mobilecontent #mobileparamstable #name").val(data["Name"]);
					}
				}
			});

			$('#modal').hide();
		}

		ShowUsers = function () {
			var oTable;

			$('#modal').show();

			$.devIdx = -1;

			var htmlcontent = "";
			htmlcontent += $('#mobilemain').html();
			$('#mobilecontent').html(htmlcontent);
			$('#mobilecontent').i18n();

			oTable = $('#mobiletable').dataTable({
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aoColumnDefs": [
					{ "bSortable": false, "aTargets": [0] }
				],
				"aaSorting": [[1, "asc"]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});

			$('#modal').hide();
			RefreshMobileTable();
		}

		init();

		function init() {
			$scope.MakeGlobalConfig();
			//Get devices
			$("#userdevices #userdevicestable #devices").html("");
			$.ajax({
				url: "json.htm?type=command&param=devices_list",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var option = $('<option />');
							option.attr('value', item.value).text(item.name);
							$("#userdevices #userdevicestable #devices").append(option);
						});
					}
				}
			});

			ShowUsers();
		};
	}]);
});