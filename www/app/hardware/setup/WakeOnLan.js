define(['app'], function (app) {
    app.component('wolHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/WakeOnLan.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

            var oTable = $('#nodestable').dataTable({
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

            RefreshWOLNodeTable();
        };

        AddWOLNode = function () {
            var name = $("#hardwarecontent #nodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var mac = $("#hardwarecontent #nodeparamstable #nodemac").val();
            if (mac == "") {
                ShowNotify($.t('Please enter a MAC Address!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=woladdnode" +
                "&idx=" + $.devIdx +
                "&name=" + encodeURIComponent(name) +
                "&mac=" + mac,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshWOLNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        WOLDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=wolremovenode" +
                        "&idx=" + $.devIdx +
                        "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshWOLNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        WOLClearNodes = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=wolclearnodes" +
                        "&idx=" + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshWOLNodeTable();
                        }
                    });
                }
            });
        }

        WOLUpdateNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }

            var name = $("#hardwarecontent #nodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var mac = $("#hardwarecontent #nodeparamstable #nodemac").val();
            if (mac == "") {
                ShowNotify($.t('Please enter a MAC Address!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=wolupdatenode" +
                "&idx=" + $.devIdx +
                "&nodeid=" + nodeid +
                "&name=" + encodeURIComponent(name) +
                "&mac=" + mac,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshWOLNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshWOLNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #nodeparamstable #nodename").val("");
            $("#hardwarecontent #nodeparamstable #nodemac").val("");

            var oTable = $('#nodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=wolgetnodes&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.idx,
                                "Name": item.Name,
                                "Mac": item.Mac,
                                "0": item.idx,
                                "1": item.Name,
                                "2": item.Mac
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#nodestable tbody").off();
            $("#nodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #nodeparamstable #nodename").val("");
                    $("#hardwarecontent #nodeparamstable #nodemac").val("");
                }
                else {
                    var oTable = $('#nodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:WOLUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:WOLDeleteNode(" + idx + ")");
                        $("#hardwarecontent #nodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #nodeparamstable #nodemac").val(data["2"]);
                    }
                }
            });

            $('#modal').hide();
        }
    }
});
