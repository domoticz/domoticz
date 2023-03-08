define(['app'], function (app) {
    app.component('mqttadHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/MQTT-AD.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

            var oTable = $('#numberstable').dataTable({
                "sDom": '<"H"lfrC>t<"F"ip>',
                "oTableTools": {
                    "sRowSelect": "single",
                },
                "aaSorting": [[0, "desc"]],
                "bSortClasses": false,
                "bProcessing": true,
                "bStateSave": true,
                "bJQueryUI": true,
                "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
                "iDisplayLength": 25,
                "sPaginationType": "full_numbers",
                language: $.DataTableLanguage
            });

            $('#hardwarecontent #idx').val($ctrl.hardware.idx);

            RefreshConfiguration();
        };

        UpdateNumber = function (numid) {
            if ($('#numbervaluetable #numberupdate').attr("class") == "btnstyle3-dis") {
                return;
            }

            var value = $("#numbervaluetable #numval").val();
            if (value == "") {
                ShowNotify($.t('Please enter a valid integer'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=mqttupdatenumber" +
                "&idx=" + $.devIdx +
                "&name=" + encodeURIComponent(numid) +
                "&value=" + parseFloat(value),
                async: false,
                dataType: 'json',
                success: function (data) {
					ShowNotify($.t('It could take some seconds before the value is applied. Refresh the table manually!'), 5000);
                    //RefreshConfiguration();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshConfiguration = function () {
            $('#modal').show();
            $('#numbervaluetable #numberupdate').attr("class", "btnstyle3-dis");

            $("#numbervaluetable #numval").val("");
            document.getElementById("numunit").innerText = "";

            var oTable = $('#numberstable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=mqttadgetconfig&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.idx,
                                "Device": item.dev_name,
                                "Name": item.name,
                                "Value": item.value,
                                "Unit": item.unit,
                                "min": item.min,
                                "max": item.max,
                                "step": item.step,
                                "0": item.dev_name,
                                "1": item.name,
                                "2": (item.value!="") ? item.value : "Unknown",
                                "3": item.unit
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#numberstable tbody").off();
            $("#numberstable tbody").on('click', 'tr', function () {
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#numbervaluetable #numberupdate').attr("class", "btnstyle3-dis");
					$("#numbervaluetable #numval").val("");
					document.getElementById("numunit").innerText = "";
                }
                else {
                    var oTable = $('#numberstable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#numbervaluetable #numberupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var id = data["DT_RowId"];
                        $("#numbervaluetable #numberupdate").attr("href", "javascript:UpdateNumber('" + id + "')");
                        $('#numbervaluetable #numberupdate').attr("class", "btnstyle3");

						var numobj = document.getElementById("numval");
						
						var value = data["2"];
						if (value == "Unknown") value = "";
						numobj.value = value;
						numobj.min = data["min"];
						numobj.max = data["max"];
						numobj.step = data["step"];
						document.getElementById("numunit").innerText = data["3"];
                    }
                }
            });

            $('#modal').hide();
        }
    }
});
