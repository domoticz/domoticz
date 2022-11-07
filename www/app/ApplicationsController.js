define(['app'], function (app) {
	app.controller('ApplicationsController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'md5', function ($scope, $rootScope, $location, $http, $interval, md5) {

		DeleteApplication = function (idx) {
			bootbox.confirm($.t("Are you sure you want to delete this Application?"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deleteapplication&idx=" + idx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshApplicationTable();
						},
						error: function () {
							HideNotify();
							ShowNotify($.t('Problem deleting Application!'), 2500, true);
						}
					});
				}
			});
		}

		GetApplicationSettings = function () {
			var csettings = {};

			csettings.bEnabled = $('#applicationcontent #applicationparamstable #enabled').is(":checked");
			csettings.applicationname = $("#applicationcontent #applicationparamstable #applicationname").val();
			if (csettings.applicationname == "") {
				ShowNotify($.t('Please enter an Applicationname!'), 2500, true);
				return;
			}
			csettings.bPublic = $('#applicationcontent #applicationparamstable #applicationpublic').is(":checked");
			csettings.secret = $("#applicationcontent #applicationparamstable #applicationsecret").val();
			csettings.pemfile = $("#applicationcontent #applicationparamstable #applicationpemfile").val();
			if ((csettings.bPublic == false) && (csettings.secret == "")) {
				ShowNotify($.t('Please enter a Secret!'), 2500, true);
				return;
			}
			if ((csettings.bPublic == true) && (csettings.pemfile == "")) {
				ShowNotify($.t('Please enter a PEM file!'), 2500, true);
				return;
			}
			if ((csettings.secret != "") && (csettings.secret.length != 32)) {
				csettings.secret = md5.createHash(csettings.secret);
			}

			return csettings;
		}

		UpdateApplication = function (idx) {
			var csettings = GetApplicationSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				url: "json.htm?type=command&param=updateapplication&idx=" + idx +
				"&enabled=" + csettings.bEnabled +
				"&applicationname=" + csettings.applicationname +
				"&secret=" + csettings.secret +
				"&pemfile=" + csettings.pemfile +
				"&public=" + csettings.bPublic,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "ERR") {
						ShowNotify(data.statustext, 2500, true);
						return;
					}
					RefreshApplicationTable();
				},
				error: function () {
					ShowNotify($.t('Problem updating Application!'), 2500, true);
				}
			});
		}

		AddApplication = function () {
			var csettings = GetApplicationSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				url: "json.htm?type=command&param=addapplication&enabled=" + csettings.bEnabled +
				"&applicationname=" + csettings.applicationname +
				"&secret=" + csettings.secret +
				"&pemfile=" + csettings.pemfile +
				"&public=" + csettings.bPublic,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status != "OK") {
						ShowNotify(data.statustext, 2500, true);
						return;
					}
					RefreshApplicationTable();
				},
				error: function () {
					ShowNotify($.t('Problem adding Application!'), 2500, true);
				}
			});
		}

		RefreshApplicationTable = function () {
			$('#modal').show();

			$.devIdx = -1;

			$('#updelclr #applicationupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #applicationdelete').attr("class", "btnstyle3-dis");

			var oTable = $('#applicationtable').dataTable();
			oTable.fnClearTable();
			$.ajax({
				url: "json.htm?type=command&param=getapplications",
				async: false,
				dataType: 'json',
				success: function (data) {

					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var enabledstr = $.t("No");
							if (item.Enabled == "true") {
								enabledstr = $.t("Yes");
							}
							var publicstr = $.t("No");
							if (item.Public == "true") {
								publicstr = $.t('Yes');
							}
							var addId = oTable.fnAddData({
								"DT_RowId": item.idx,
								"Enabled": item.Enabled,
								"Applicationname": item.Applicationname,
								"Applicationsecret": item.Secret,
								"Applicationpemfile": item.Pemfile,
								"Public": item.Public,
								"Last seen": item.LastSeen,
								"0": enabledstr,
								"1": item.Applicationname,
								"2": publicstr,
								"3": item.LastSeen
							});
					});
					}
				}
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#applicationtable tbody").off();
			$("#applicationtable tbody").on('click', 'tr', function () {
				$.devIdx = -1;

				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$('#updelclr #applicationupdate').attr("class", "btnstyle3-dis");
					$('#updelclr #applicationdelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#applicationtable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#updelclr #applicationupdate').attr("class", "btnstyle3");
					$('#updelclr #applicationdelete').attr("class", "btnstyle3");
					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						$.devIdx = idx;
						$("#updelclr #applicationupdate").attr("href", "javascript:UpdateApplication(" + idx + ")");
						$("#updelclr #applicationdelete").attr("href", "javascript:DeleteApplication(" + idx + ")");
						$('#applicationcontent #applicationparamstable #enabled').prop('checked', (data["Enabled"] == "true"));
						$("#applicationcontent #applicationparamstable #applicationname").val(data["Applicationname"]);
						$("#applicationcontent #applicationparamstable #applicationsecret").val(data["Applicationsecret"]);
						$("#applicationcontent #applicationparamstable #applicationpemfile").val(data["Applicationpemfile"]);
						$('#applicationcontent #applicationparamstable #applicationpublic').prop('checked', (data["Public"] == "true"));
						togglePublic();
					}
				}
			});

			$('#modal').hide();
		}

		ShowApplications = function () {
			var oTable;

			$('#modal').show();

			$.devIdx = -1;

			var htmlcontent = "";
			htmlcontent += $('#applicationmain').html();
			$('#applicationcontent').html(htmlcontent);
			$('#applicationcontent').i18n();

			oTable = $('#applicationtable').dataTable({
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
			RefreshApplicationTable();
		}

		togglePublic = function () {
			var isPublic = $('#applicationcontent #applicationparamstable #applicationpublic').is(":checked");
			if (isPublic)
			{
				$('#applicationcontent #applicationparamstable #apppemfiletr').show();
				$('#applicationcontent #applicationparamstable #appsecrettr').hide();
				$('#applicationcontent #applicationparamstable #applicationsecret').val("");
			}
			else
			{
				$('#applicationcontent #applicationparamstable #appsecrettr').show();
				$('#applicationcontent #applicationparamstable #apppemfiletr').hide();
				$('#applicationcontent #applicationparamstable #applicationpemfile').val("");
			}
		}

		init();

		function init() {
			$scope.MakeGlobalConfig();

			ShowApplications();
		};
	}]);
});