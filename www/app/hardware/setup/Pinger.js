define(['app'], function (app) {
    app.component('pingerHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/Pinger.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

            $("#hardwarecontent #pingsettingstable #pollinterval").val($ctrl.hardware.Mode1);
            $("#hardwarecontent #pingsettingstable #pingtimeout").val($ctrl.hardware.Mode2);

            var oTable = $('#ipnodestable').dataTable({
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

            RefreshPingerNodeTable();
        };

        AddPingerNode = function () {
            var name = $("#hardwarecontent #ipnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #ipnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Timeout = parseInt($("#hardwarecontent #ipnodeparamstable #nodetimeout").val());
            if (Timeout < 1)
                Timeout = 5;

            $.ajax({
                url: "json.htm?type=command&param=pingeraddnode" +
                "&idx=" + $.devIdx +
                "&name=" + encodeURIComponent(name) +
                "&ip=" + ip +
                "&timeout=" + Timeout,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshPingerNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        PingerDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=pingerremovenode" +
                        "&idx=" + $.devIdx +
                        "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshPingerNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        PingerClearNodes = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=pingerclearnodes" +
                        "&idx=" + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshPingerNodeTable();
                        }
                    });
                }
            });
        }

        PingerUpdateNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }

            var name = $("#hardwarecontent #ipnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #ipnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Timeout = parseInt($("#hardwarecontent #ipnodeparamstable #nodetimeout").val());
            if (Timeout < 1)
                Timeout = 5;

            $.ajax({
                url: "json.htm?type=command&param=pingerupdatenode" +
                "&idx=" + $.devIdx +
                "&nodeid=" + nodeid +
                "&name=" + encodeURIComponent(name) +
                "&ip=" + ip +
                "&timeout=" + Timeout,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshPingerNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshPingerNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #ipnodeparamstable #nodename").val("");
            $("#hardwarecontent #ipnodeparamstable #nodeip").val("");
            $("#hardwarecontent #ipnodeparamstable #nodetimeout").val("5");

            var oTable = $('#ipnodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=pingergetnodes&idx=" + $.devIdx,
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
                                "3": item.Timeout
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#ipnodestable tbody").off();
            $("#ipnodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #ipnodeparamstable #nodename").val("");
                    $("#hardwarecontent #ipnodeparamstable #nodeip").val("");
                    $("#hardwarecontent #ipnodeparamstable #nodetimeout").val("5");
                }
                else {
                    var oTable = $('#ipnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:PingerUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:PingerDeleteNode(" + idx + ")");
                        $("#hardwarecontent #ipnodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #ipnodeparamstable #nodeip").val(data["2"]);
                        $("#hardwarecontent #ipnodeparamstable #nodetimeout").val(data["3"]);
                    }
                }
            });

            $('#modal').hide();
        }

        SetPingerSettings = function () {
            var Mode1 = parseInt($("#hardwarecontent #pingsettingstable #pollinterval").val());
            if (Mode1 < 1)
                Mode1 = 30;
            var Mode2 = parseInt($("#hardwarecontent #pingsettingstable #pingtimeout").val());
            if (Mode2 < 500)
                Mode2 = 500;
            $.ajax({
                url: "json.htm?type=command&param=pingersetmode" +
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
