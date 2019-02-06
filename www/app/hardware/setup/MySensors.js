define(['app'], function (app) {
    app.component('mySensorsHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/MySensors.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

            $('#mysensorsnodestable').dataTable({
                "sDom": '<"H"lfrC>t<"F"ip>',
                "oTableTools": {
                    "sRowSelect": "single"
                },
                "aaSorting": [[0, "asc"]],
                "bSortClasses": false,
                "bProcessing": true,
                "bStateSave": true,
                "bJQueryUI": true,
                "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
                "iDisplayLength": 25,
                "sPaginationType": "full_numbers",
                language: $.DataTableLanguage
            });
            $('#mysensorsactivetable').dataTable({
                "sDom": '<"H"lfrC>t<"F"ip>',
                "oTableTools": {
                    "sRowSelect": "single"
                },
                "aaSorting": [[0, "asc"]],
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
            RefreshMySensorsNodeTable();
        };

        MySensorsUpdateNode = function (nodeid) {
            if ($('#updelclr #nodeupdate').attr("class") == "btnstyle3-dis") {
                return;
            }
            var name = $("#updelclr #nodename").val();
            $.ajax({
                url: "json.htm?type=command&param=mysensorsupdatenode" +
                "&idx=" + $.devIdx +
                "&nodeid=" + nodeid +
                "&name=" + encodeURIComponent(name),
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshMySensorsNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        MySensorsDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=mysensorsremovenode" +
                        "&idx=" + $.devIdx +
                        "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshMySensorsNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        MySensorsDeleteChild = function (nodeid, childid) {
            if ($('#updelclr #activedevicedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Child?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=mysensorsremovechild" +
                        "&idx=" + $.devIdx +
                        "&nodeid=" + nodeid +
                        "&childid=" + childid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            MySensorsRefreshActiveDevicesTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Child!'), 2500, true);
                        }
                    });
                }
            });
        }
        MySensorsUpdateChild = function (nodeid, childid) {
            if ($('#updelclr #activedeviceupdate').attr("class") == "btnstyle3-dis") {
                return;
            }
            var bUseAck = $('#hardwarecontent #mchildsettings #Ack').is(":checked");
            var AckTimeout = parseInt($('#hardwarecontent #mchildsettings #AckTimeout').val());
            if (AckTimeout < 100)
                AckTimeout = 100;
            else if (AckTimeout > 10000)
                AckTimeout = 10000;
            $.ajax({
                url: "json.htm?type=command&param=mysensorsupdatechild" +
                "&idx=" + $.devIdx +
                "&nodeid=" + nodeid +
                "&childid=" + childid +
                "&useack=" + bUseAck +
                "&acktimeout=" + AckTimeout,
                async: false,
                dataType: 'json',
                success: function (data) {
                    MySensorsRefreshActiveDevicesTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Child!'), 2500, true);
                }
            });
        }

        MySensorsRefreshActiveDevicesTable = function () {
            //$('#plancontent #delclractive #activedevicedelete').attr("class", "btnstyle3-dis");
            $('#hardwarecontent #trChildSettings').hide();
            var oTable = $('#mysensorsactivetable').dataTable();
            oTable.fnClearTable();
            if ($.nodeid == -1) {
                return;
            }
            $.ajax({
                url: "json.htm?type=command&param=mysensorsgetchilds" +
                "&idx=" + $.devIdx +
                "&nodeid=" + $.nodeid,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        var totalItems = data.result.length;
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.child_id,
                                "AckEnabled": item.use_ack,
                                "AckTimeout": item.ack_timeout,
                                "0": item.child_id,
                                "1": item.type,
                                "2": item.name,
                                "3": item.Values,
                                "4": item.use_ack,
                                "5": item.ack_timeout,
                                "6": item.LastReceived
                            });
                        });
                    }
                }
            });
            /* Add a click handler to the rows - this could be used as a callback */
            $("#mysensorsactivetable tbody").off();
            $("#mysensorsactivetable tbody").on('click', 'tr', function () {
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#delclractive #activedevicedelete').attr("class", "btnstyle3-dis");
                    $('#delclractive #activedeviceupdate').attr("class", "btnstyle3-dis");
                    $('#hardwarecontent #trChildSettings').hide();
                }
                else {
                    var oTable = $('#mysensorsactivetable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#activedevicedelete').attr("class", "btnstyle3");
                    $('#activedeviceupdate').attr("class", "btnstyle3");
                    $('#hardwarecontent #trChildSettings').show();
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $('#hardwarecontent #mchildsettings #Ack').prop('checked', (data["AckEnabled"] == "true"));
                        $('#hardwarecontent #mchildsettings #AckTimeout').val(data["AckTimeout"]);
                        $("#activedevicedelete").attr("href", "javascript:MySensorsDeleteChild(" + $.nodeid + "," + idx + ")");
                        $("#activedeviceupdate").attr("href", "javascript:MySensorsUpdateChild(" + $.nodeid + "," + idx + ")");
                    }
                }
            });

            $('#modal').hide();
        }

        RefreshMySensorsNodeTable = function () {
            $('#hardwarecontent #trChildSettings').hide();
            $('#updelclr #trNodeSettings').hide();
            $('#modal').show();
            var oTable = $('#mysensorsnodestable').dataTable();
            oTable.fnClearTable();
            $.nodeid = -1;
            $.ajax({
                url: "json.htm?type=command&param=mysensorsgetnodes&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.idx,
                                "Name": item.Name,
                                "SketchName": item.SketchName,
                                "Version": item.Version,
                                "0": item.idx,
                                "1": item.Name,
                                "2": item.SketchName,
                                "3": item.Version,
                                "4": item.Childs,
                                "5": item.LastReceived
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#mysensorsnodestable tbody").off();
            $("#mysensorsnodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                $('#updelclr #trNodeSettings').hide();
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                }
                else {
                    var oTable = $('#mysensorsnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                        $('#updelclr #nodename').val(data["Name"]);
                        $('#updelclr #trNodeSettings').show();
                        $("#updelclr #nodeupdate").attr("href", "javascript:MySensorsUpdateNode(" + idx + ")");
                        $("#updelclr #nodedelete").attr("href", "javascript:MySensorsDeleteNode(" + idx + ")");
                        $.nodeid = idx;
                        MySensorsRefreshActiveDevicesTable();
                    }
                }
            });

            $('#modal').hide();
        }
    }
});
