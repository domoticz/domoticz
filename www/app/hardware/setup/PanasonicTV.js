define(['app'], function (app) {
    app.component('panasonicTvHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/PanasonicTV.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

            $("#hardwarecontent #panasonicsettingstable #pollinterval").val($ctrl.hardware.Mode1);
            $("#hardwarecontent #panasonicsettingstable #pingtimeout").val($ctrl.hardware.Mode2);
            $("#hardwarecontent #panasonicsettingstable #unknowncommands").prop('checked', $ctrl.hardware.Mode3 & 1);
            $("#hardwarecontent #panasonicsettingstable #tryifoff").prop('checked', $ctrl.hardware.Mode3 & 2);
            $("#hardwarecontent #panasonicsettingstable #custombuttons").val($ctrl.hardware.Extra);

            $('#panasonicnodestable').dataTable({
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

            $('#hardwarecontent #idx').val(idx);

            RefreshPanasonicNodeTable();
        };

        PanasonicAddNode = function () {
            var name = $("#hardwarecontent #panasonicnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #panasonicnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($("#hardwarecontent #panasonicnodeparamstable #nodeport").val());
            if (Port == "") {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=panasonicaddnode" +
                "&idx=" + $.devIdx +
                "&name=" + encodeURIComponent(name) +
                "&ip=" + ip +
                "&port=" + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshPanasonicNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        PanasonicDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=panasonicremovenode" +
                        "&idx=" + $.devIdx +
                        "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshPanasonicNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        PanasonicClearNodes = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=panasonicclearnodes" +
                        "&idx=" + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshPanasonicNodeTable();
                        }
                    });
                }
            });
        }

        PanasonicUpdateNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }

            var name = $("#hardwarecontent #panasonicnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #panasonicnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($("#hardwarecontent #panasonicnodeparamstable #nodeport").val());
            if (Port == "") {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=panasonicupdatenode" +
                "&idx=" + $.devIdx +
                "&nodeid=" + nodeid +
                "&name=" + encodeURIComponent(name) +
                "&ip=" + ip +
                "&port=" + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshPanasonicNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshPanasonicNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #panasonicnodeparamstable #nodename").val("");
            $("#hardwarecontent #panasonicnodeparamstable #nodeip").val("");
            $("#hardwarecontent #panasonicnodeparamstable #nodeport").val("55000");

            var oTable = $('#panasonicnodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=panasonicgetnodes&idx=" + $.devIdx,
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
                                "3": item.Port
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#panasonicnodestable tbody").off();
            $("#panasonicnodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #panasonicnodeparamstable #nodename").val("");
                    $("#hardwarecontent #panasonicnodeparamstable #nodeip").val("");
                    $("#hardwarecontent #panasonicnodeparamstable #nodeport").val("55000");
                }
                else {
                    var oTable = $('#panasonicnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:PanasonicUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:PanasonicDeleteNode(" + idx + ")");
                        $("#hardwarecontent #panasonicnodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #panasonicnodeparamstable #nodeip").val(data["2"]);
                        $("#hardwarecontent #panasonicnodeparamstable #nodeport").val(data["3"]);
                    }
                }
            });

            $('#modal').hide();
        }

        SetPanasonicSettings = function () {
            var Mode1 = parseInt($("#hardwarecontent #panasonicsettingstable #pollinterval").val());
            if (Mode1 < 1)
                Mode1 = 30;
            var Mode2 = parseInt($("#hardwarecontent #panasonicsettingstable #pingtimeout").val());
            if (Mode2 < 500)
                Mode2 = 500;
            var Mode3 = 0;
            Mode3 |= ($("#hardwarecontent #panasonicsettingstable #unknowncommands").is(':checked'))?1:0;
            Mode3 |= ($("#hardwarecontent #panasonicsettingstable #tryifoff").is(':checked'))?2:0;
            var Extra = $("#hardwarecontent #panasonicsettingstable #custombuttons").val();
            $.ajax({
                url: "json.htm?type=command&param=panasonicsetmode" +
                "&idx=" + $.devIdx +
                "&mode1=" + Mode1 +
                "&mode2=" + Mode2 +
                "&mode3=" + Mode3 +
                "&extra=" + Extra ,
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
