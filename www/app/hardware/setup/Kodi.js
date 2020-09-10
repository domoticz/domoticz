define(['app'], function (app) {
    app.component('kodiHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/Kodi.html',
        controller: KodiHardwareController
    });

    function KodiHardwareController() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            EditKodi($ctrl.hardware.idx, $ctrl.hardware.Name, $ctrl.hardware.Mode1, $ctrl.hardware.Mode2)
        };

        AddKodiNode = function () {
            var name = $('#hardwarecontent #kodinodeparamstable #nodename').val();
            if (name == '') {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $('#hardwarecontent #kodinodeparamstable #nodeip').val();
            if (ip == '') {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($('#hardwarecontent #kodinodeparamstable #nodeport').val());
            if (Port == '') {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: 'json.htm?type=command&param=kodiaddnode' +
                '&idx=' + $.devIdx +
                '&name=' + encodeURIComponent(name) +
                '&ip=' + ip +
                '&port=' + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshKodiNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        KodiDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr('class') == 'btnstyle3-dis') {
                return;
            }
            bootbox.confirm($.t('Are you sure to remove this Node?'), function (result) {
                if (result == true) {
                    $.ajax({
                        url: 'json.htm?type=command&param=kodiremovenode' +
                        '&idx=' + $.devIdx +
                        '&nodeid=' + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshKodiNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        KodiClearNodes = function () {
            bootbox.confirm($.t('Are you sure to delete ALL Nodes?\n\nThis action can not be undone!'), function (result) {
                if (result == true) {
                    $.ajax({
                        url: 'json.htm?type=command&param=kodiclearnodes' +
                        '&idx=' + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshKodiNodeTable();
                        }
                    });
                }
            });
        }

        KodiUpdateNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr('class') == 'btnstyle3-dis') {
                return;
            }

            var name = $('#hardwarecontent #kodinodeparamstable #nodename').val();
            if (name == '') {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $('#hardwarecontent #kodinodeparamstable #nodeip').val();
            if (ip == '') {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($('#hardwarecontent #kodinodeparamstable #nodeport').val());
            if (Port == '') {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: 'json.htm?type=command&param=kodiupdatenode' +
                '&idx=' + $.devIdx +
                '&nodeid=' + nodeid +
                '&name=' + encodeURIComponent(name) +
                '&ip=' + ip +
                '&port=' + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshKodiNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshKodiNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
            $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
            $('#hardwarecontent #kodinodeparamstable #nodename').val('');
            $('#hardwarecontent #kodinodeparamstable #nodeip').val('');
            $('#hardwarecontent #kodinodeparamstable #nodeport').val('9090');

            var oTable = $('#kodinodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: 'json.htm?type=command&param=kodigetnodes&idx=' + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                'DT_RowId': item.idx,
                                'Name': item.Name,
                                'IP': item.IP,
                                '0': item.idx,
                                '1': item.Name,
                                '2': item.IP,
                                '3': item.Port
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $('#kodinodestable tbody').off();
            $('#kodinodestable tbody').on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
                    $('#hardwarecontent #kodinodeparamstable #nodename').val('');
                    $('#hardwarecontent #kodinodeparamstable #nodeip').val('');
                    $('#hardwarecontent #kodinodeparamstable #nodeport').val('9090');
                }
                else {
                    var oTable = $('#kodinodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr('class', 'btnstyle3');
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data['DT_RowId'];
                        $('#updelclr #nodeupdate').attr('href', 'javascript:KodiUpdateNode(' + idx + ')');
                        $('#updelclr #nodedelete').attr('class', 'btnstyle3');
                        $('#updelclr #nodedelete').attr('href', 'javascript:KodiDeleteNode(' + idx + ')');
                        $('#hardwarecontent #kodinodeparamstable #nodename').val(data['1']);
                        $('#hardwarecontent #kodinodeparamstable #nodeip').val(data['2']);
                        $('#hardwarecontent #kodinodeparamstable #nodeport').val(data['3']);
                    }
                }
            });

            $('#modal').hide();
        }

        SetKodiSettings = function () {
            var Mode1 = parseInt($('#hardwarecontent #kodisettingstable #pollinterval').val());
            if (Mode1 < 1)
                Mode1 = 30;
            var Mode2 = parseInt($('#hardwarecontent #kodisettingstable #pingtimeout').val());
            if (Mode2 < 500)
                Mode2 = 500;
            $.ajax({
                url: 'json.htm?type=command&param=kodisetmode' +
                '&idx=' + $.devIdx +
                '&mode1=' + Mode1 +
                '&mode2=' + Mode2,
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

        function EditKodi(idx, name, Mode1, Mode2) {
            $.devIdx = idx;

            $('#hardwarecontent #kodisettingstable #pollinterval').val(Mode1);
            $('#hardwarecontent #kodisettingstable #pingtimeout').val(Mode2);

            var oTable = $('#kodinodestable').dataTable({
                'sDom': '<"H"lfrC>t<"F"ip>',
                'oTableTools': {
                    'sRowSelect': 'single',
                },
                'aaSorting': [[0, 'desc']],
                'bSortClasses': false,
                'bProcessing': true,
                'bStateSave': true,
                'bJQueryUI': true,
                'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, 'All']],
                'iDisplayLength': 25,
                'sPaginationType': 'full_numbers',
                language: $.DataTableLanguage
            });

            $('#hardwarecontent #idx').val(idx);

            RefreshKodiNodeTable();
        }
    }
});
