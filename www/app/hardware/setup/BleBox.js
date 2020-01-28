define([ "app" ], function(app) {
    app.component("bleBoxHardware", {
        bindings: {
            hardware: "<"
        },
        templateUrl: "app/hardware/setup/BleBox.html",
        controller: Controller
    });
    function Controller() {
        var $ctrl = this;
        $ctrl.$onInit = function() {
            $.hw_id = $ctrl.hardware.idx;
            $("#hardwarecontent #bleboxsettingstable #pollinterval").val($ctrl.hardware.Mode1);
            $("#bleboxfeaturestable").dataTable({
                sDom: '<"H"lfrC>t<"F"ip>',
                oTableTools: {
                    sRowSelect: "single"
                },
                aaSorting: [ [ 0, "desc" ] ],
                bSortClasses: false,
                bProcessing: true,
                bStateSave: true,
                bJQueryUI: true,
                aLengthMenu: [ [ 25, 50, 100, -1 ], [ 25, 50, 100, "All" ] ],
                iDisplayLength: 25,
                sPaginationType: "full_numbers",
                language: $.DataTableLanguage
            });
            $("#hardwarecontent #idx").val($.hw_id);
            RefreshBleBoxFeaturesTable();
            var save_button = $("#blebox_features_save");
            if (save_button.hasClass("invisible")) {
                save_button.removeClass("invisible");
            }
        };
        BleBoxRemoveFeature = function(nodeid) {
            if ($("#updelclr #feature_delete").attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this feature?"), function(result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=bleboxremovefeature" + "&hw_id=" + $.hw_id + "&nodeid=" + nodeid,
                        async: false,
                        dataType: "json",
                        success: function(data) {
                            RefreshBleBoxFeaturesTable();
                        },
                        error: function() {
                            ShowNotify($.t("Problem removing feature!"), 2500, true);
                        }
                    });
                }
            });
        };
        BleBoxClearFeatures = function() {
            bootbox.confirm($.t("Are you sure to delete ALL features?\n\nThis action can not be undone!"), function(result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=bleboxclearfeatures" + "&hw_id=" + $.hw_id,
                        async: false,
                        dataType: "json",
                        success: function(data) {
                            RefreshBleBoxFeaturesTable();
                        }
                    });
                }
            });
        };
        RefreshBleBoxFeaturesTable = function() {
            $("#updelclr #feature_delete").attr("class", "btnstyle3-dis");
            $("#bleboxdeviceinfo #device_name").val("");
            $("#bleboxdeviceinfo #device_type").val("");
            $("#bleboxdeviceinfo #device_uptime").val("");
            $("#bleboxdeviceinfo #device_hardware_version").val("");
            $("#bleboxdeviceinfo #device_firmware_version").val("");
            var oTable = $("#bleboxfeaturestable").dataTable();
            oTable.fnClearTable();
            $.ajax({
                dataType: "json",
                url: "json.htm?type=command&param=bleboxgetfeatures&hw_id=" + $.hw_id,
                async: false,
                success: function(data) {
                    if (typeof data.result != "undefined") {
                        var dev = data.result["device_info"];
                        $("#bleboxdeviceinfo #device_name").text(dev.name);
                        $("#bleboxdeviceinfo #device_type").text(dev.type);
                        $("#bleboxdeviceinfo #device_uptime").text(dev.uptime);
                        $("#bleboxdeviceinfo #device_hardware_version").text(dev.hv);
                        $("#bleboxdeviceinfo #device_firmware_version").text(dev.fv);
                        $.each(data.result["features"], function(i, item) {
                            var addId = oTable.fnAddData({
                                DT_RowId: item.idx,
                                0: item.idx,
                                1: item.Name,
                                2: item.Value,
                                3: item.Description
                            });
                        });
                    }
                }
            });
            $("#bleboxfeaturestable tbody").off();
            $("#bleboxfeaturestable tbody").on("click", "tr", function() {
                $("#updelclr #feature_delete").attr("class", "btnstyle3-dis");
                if ($(this).hasClass("row_selected")) {
                    $(this).removeClass("row_selected");
                } else {
                    var oTable = $("#bleboxfeaturestable").dataTable();
                    oTable.$("tr.row_selected").removeClass("row_selected");
                    $(this).addClass("row_selected");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $("#updelclr #feature_delete").attr("class", "btnstyle3");
                        $("#updelclr #feature_delete").attr("href", "javascript:BleBoxRemoveFeature(" + idx + ")");
                    }
                }
            });
            $("#modal").hide();
        };
        BleBoxDetectFeatures = function() {
            $.ajax({
                url: "json.htm?type=command&param=bleboxdetectfeatures" + "&hw_id=" + $.hw_id,
                async: false,
                dataType: "json",
                success: function(data) {
                    RefreshBleBoxFeaturesTable();
                }
            });
        };
        BleBoxRefreshFeatures = function() {
            $.ajax({
                url: "json.htm?type=command&param=bleboxrefreshfeatures" + "&hw_id=" + $.hw_id,
                async: false,
                dataType: "json",
                success: function(data) {
                    RefreshBleBoxFeaturesTable();
                }
            });
        };
        BleBoxUseFeatures = function() {
            $.ajax({
                url: "json.htm?type=command&param=bleboxusefeatures" + "&hw_id=" + $.hw_id,
                async: false,
                dataType: "json",
                success: function(data) {
                    window.location.href = "#Devices";
                }
            });
        };
    }
});
