define(['app'], function (app) {
    app.component('bleBoxHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/BleBox.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

            $("#hardwarecontent #bleboxsettingstable #pollinterval").val($ctrl.hardware.Mode1);
            $("#hardwarecontent #bleboxsettingstable #pingtimeout").val($ctrl.hardware.Mode2);
            $("#hardwarecontent #bleboxsettingstable #ipmask").val("192.168.1.*");

            $('#bleboxnodestable').dataTable({
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

            RefreshBleBoxNodeTable();
        };

        BleBoxAddNode = function () {
            var name = $("#hardwarecontent #bleboxnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #bleboxnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=bleboxaddnode" +
                "&idx=" + $.devIdx +
                "&name=" + encodeURIComponent(name) +
                "&ip=" + ip,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshBleBoxNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        BleBoxDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=bleboxremovenode" +
                        "&idx=" + $.devIdx +
                        "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshBleBoxNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        BleBoxClearNodes = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=bleboxclearnodes" +
                        "&idx=" + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshBleBoxNodeTable();
                        }
                    });
                }
            });
        }

        BleBoxAutoSearchingNodes = function () {

            var ipmask = $("#hardwarecontent #bleboxsettingstable #ipmask").val();
            if (ipmask == "") {
                ShowNotify($.t('Please enter a mask!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=bleboxautosearchingnodes" +
                "&idx=" + $.devIdx +
                "&ipmask=" + ipmask,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshBleBoxNodeTable();
                }
            });
        }

        BleBoxUpdateFirmware = function () {

            ShowNotify($.t('Please wait. Updating ....!'), 2500, true);

            $.ajax({
                url: "json.htm?type=command&param=bleboxupdatefirmware" +
                "&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshBleBoxNodeTable();
                }
            });
        }

        RefreshBleBoxNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #bleboxnodeparamstable #nodename").val("");
            $("#hardwarecontent #bleboxnodeparamstable #nodeip").val("");


            var oTable = $('#bleboxnodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=bleboxgetnodes&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.idx,
                                "Name": item.Name,
                                "IP": item.IP,
                                "0": item.idx,
                                "1": item.Name,
                                "2": item.IP,
                                "3": item.Type,
                                "4": item.Uptime,
                                "5": item.hv,
                                "6": item.fv
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#bleboxnodestable tbody").off();
            $("#bleboxnodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $("#hardwarecontent #bleboxnodeparamstable #nodename").val("");
                    $("#hardwarecontent #bleboxnodeparamstable #nodeip").val("");
                }
                else {
                    var oTable = $('#bleboxnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:BleBoxDeleteNode(" + idx + ")");
                        $("#hardwarecontent #bleboxnodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #bleboxnodeparamstable #nodeip").val(data["2"]);
                    }
                }
            });

            $('#modal').hide();
        }

        SetBleBoxSettings = function () {
            var Mode1 = parseInt($("#hardwarecontent #bleboxsettingstable #pollinterval").val());
            if (Mode1 < 1)
                Mode1 = 30;
            var Mode2 = parseInt($("#hardwarecontent #bleboxsettingstable #pingtimeout").val());
            if (Mode2 < 1000)
                Mode2 = 1000;
            $.ajax({
                url: "json.htm?type=command&param=bleboxsetmode" +
                "&idx=" + $.devIdx +
                "&mode1=" + Mode1 +
                "&mode2=" + Mode2,
                async: false,
                dataType: 'json',
                success: function (data) {
                    bootbox.alert($.t('Settings saved'));
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Settings!'), 2500, true);
                }
            });
        }
    }
});
