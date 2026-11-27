define(['app'], function (app) {
    app.component('shellyHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/Shelly.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;
        var scanPollTimer = null;
        var scanPollInFlight = false;
        var destroyed = false;
        var lastScanResultKey = '';
        var editedScanNames = {};
        var selectedNode = null;
        var nodeAddresses = {};

        function ShellySetSelectedNode(node) {
            selectedNode = node;
            $('#shellyaddnodebtn').text(node ? $.t('Update') : $.t('Add'));
        }

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

            $('#shellynodestable').dataTable({
                'sDom': '<"H"lfr>t<"F"ip>',
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

            $('#hardwarecontent #idx').val($ctrl.hardware.idx);
            ShellySetSelectedNode(null);

            // Delegated click for scan-result Add buttons: no inline onclick
            $('#shellyscanresultsbody').off('click.shelly').on('click.shelly', '.shelly-add-btn', function () {
                var ip = $(this).attr('data-ip') || '';
                var port = $(this).attr('data-port') || '';
                var name = ($(this).closest('tr').find('input.shelly-scan-name').val() || '').trim();
                ShellyAddFromScan(ip, port, name);
            });
            $('#shellyscanresultsbody').off('keydown.shelly').on('keydown.shelly', '.shelly-add-btn', function (e) {
                if (e.which === 13 || e.which === 32) {
                    e.preventDefault();
                    $(this).trigger('click');
                }
            });
            $('#shellyscanresultsbody').off('input.shelly').on('input.shelly', 'input.shelly-scan-name', function () {
                editedScanNames[$(this).attr('data-key')] = $(this).val();
            });

            // Delegated click for the Delete button: no javascript: href
            $('#shellyupdelclr').off('click.shellydel').on('click.shellydel', '#shellynodedelete', function () {
                var nodeid = $(this).attr('data-nodeid');
                if (typeof nodeid !== 'undefined') {
                    ShellyDeleteNode(nodeid);
                }
            });

            ShellyRefreshNodeTable();

            // Resume polling if a scan was already running when this component mounted
            $.ajax({
                url: 'json.htm?type=command&param=shellyscanstatus&idx=' + $.devIdx,
                async: true,
                dataType: 'json',
                success: function (data) {
                    var state = data.state || 'idle';
                    if (state === 'running') {
                        ShellyUpdateScanResults(data.result || []);
                        $('#shellyscanresults').show();
                        $('#shellyscanprogress').show();
                        $('#shellyscanstartstop').text($.t('Cancel Scan'));
                        if (!destroyed && !scanPollTimer) {
                            scanPollTimer = setInterval(function () {
                                ShellyPollScanStatus();
                            }, 1000);
                        }
                    }
                }
            });
        };

        $ctrl.$onDestroy = function () {
            destroyed = true;
            // Stop the client-side timer only; the backend scan continues
            if (scanPollTimer) {
                clearInterval(scanPollTimer);
                scanPollTimer = null;
            }
        };

        ShellyAddNode = function () {
            var ip = $('#hardwarecontent #shellynodeparamstable #shellynodeip').val().trim();
            if (ip === '') {
                ShowNotify($.t('Please enter an IP Address!'), 2500, true);
                return;
            }
            var port = $('#hardwarecontent #shellynodeparamstable #shellynodeport').val().trim();
            var name = $('#hardwarecontent #shellynodeparamstable #shellynodename').val().trim();

            if (selectedNode) {
                // A node is selected in the list: apply a changed name, then a changed address. The node is captured
                // here because the selection can change while the requests run.
                var node = selectedNode;
                var nameChanged = (name !== '' && name !== node.name);
                var addressChanged = (ip !== node.ip || port !== node.port);
                if (!nameChanged && !addressChanged) {
                    ShowNotify($.t('Nothing changed'), 2500, false);
                    return;
                }
                var addressStep = function () {
                    if (!addressChanged) {
                        ShellyRefreshNodeTable();
                        return;
                    }
                    ShellySendAddNode(ip, port, nameChanged ? name : node.name, nameChanged);
                };
                if (nameChanged) {
                    $.ajax({
                        url: 'json.htm?type=command&param=shellyrenamenode&idx=' + $.devIdx + '&nodeid=' + encodeURIComponent(node.id) + '&name=' + encodeURIComponent(name),
                        async: true,
                        dataType: 'json',
                        success: function (data) {
                            if (data.status === 'ERR') {
                                var m = $('<span>').text(data.message || '').html();
                                ShowNotify($.t('Problem updating node!') + (m ? ' ' + m : ''), 2500, true);
                                return;
                            }
                            addressStep();
                        },
                        error: function () {
                            ShowNotify($.t('Problem updating node!'), 2500, true);
                        }
                    });
                } else {
                    addressStep();
                }
                return;
            }
            ShellySendAddNode(ip, port, name, false);
        };

        // The reason is shown in a dialog: it can be long and usually needs an action by the user
        function ShellyShowAddError(message) {
            var text = $('<div>').text($.t('Problem Adding Node!')).html();
            if (message) {
                text += '<br><br>' + $('<div>').text($.t(message)).html();
            }
            bootbox.alert(text);
        }

        // refreshOnError: the list is reloaded even when the request fails (a rename was already applied)
        function ShellySendAddNode(ip, port, name, refreshOnError) {
            var url = 'json.htm?type=command&param=shellyaddnode&idx=' + $.devIdx + '&ip=' + encodeURIComponent(ip);
            if (port !== '') {
                url += '&port=' + encodeURIComponent(port);
            }
            if (name !== '') {
                url += '&name=' + encodeURIComponent(name);
            }

            $.ajax({
                url: url,
                async: true,
                dataType: 'json',
                success: function (data) {
                    if (data.status === 'ERR') {
                        ShellyShowAddError(data.message);
                        if (refreshOnError) {
                            ShellyRefreshNodeTable();
                        }
                    } else {
                        ShellyRefreshNodeTable();
                    }
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                    if (refreshOnError) {
                        ShellyRefreshNodeTable();
                    }
                }
            });
        }

        ShellyDeleteNode = function (nodeid) {
            if ($('#shellyupdelclr #shellynodedelete').attr('class') === 'btnstyle3-dis') {
                return;
            }
            bootbox.confirm($.t('Are you sure to remove this Node?') + '<br><br>' + $.t('Its Domoticz devices will be deleted too.'), function (result) {
                if (result) {
                    $.ajax({
                        url: 'json.htm?type=command&param=shellyremovenode&idx=' + $.devIdx + '&nodeid=' + encodeURIComponent(nodeid),
                        async: true,
                        dataType: 'json',
                        success: function (data) {
                            ShellyRefreshNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        };

        ShellyClearNodes = function () {
            bootbox.confirm($.t('Are you sure to delete ALL Nodes?\n\nThis action can not be undone!'), function (result) {
                if (result) {
                    $.ajax({
                        url: 'json.htm?type=command&param=shellyclearnodes&idx=' + $.devIdx,
                        async: true,
                        dataType: 'json',
                        success: function (data) {
                            ShellyRefreshNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem clearing nodes!'), 2500, true);
                        }
                    });
                }
            });
        };

        ShellyRefreshNodeTable = function () {
            $('#modal').show();
            var deleteBtn = $('#shellyupdelclr #shellynodedelete');
            deleteBtn.attr('class', 'btnstyle3-dis');
            deleteBtn.removeAttr('data-nodeid');
            ShellySetSelectedNode(null);
            $('#hardwarecontent #shellynodeparamstable #shellynodename').val('');
            $('#hardwarecontent #shellynodeparamstable #shellynodeip').val('');
            $('#hardwarecontent #shellynodeparamstable #shellynodeport').val('');

            var oTable = $('#shellynodestable').DataTable();
            oTable.clear().draw();
            nodeAddresses = {};

            $.ajax({
                url: 'json.htm?type=command&param=shellygetnodes&idx=' + $.devIdx,
                async: true,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result !== 'undefined') {
                        $.each(data.result, function (i, item) {
                            var flags = '';
                            if (item.authRequired) {
                                flags += '<span class="label label-warning">' + $.t('Auth Required') + '</span> ';
                            }
                            if (item.sleepy) {
                                flags += '<span class="label label-info">' + $.t('Sleepy') + '</span>';
                            }
                            var ip = item.ip || '';
                            var ipport = ip + (item.port && item.port !== 80 ? ':' + item.port : '');
                            nodeAddresses[String(item.id)] = { ip: ip, port: (item.port && item.port !== 80) ? String(item.port) : '' };
                            oTable.row.add({
                                'DT_RowId': item.id,
                                '0': $('<div>').text(item.id !== undefined ? String(item.id) : '').html(),
                                '1': $('<div>').text(item.name || '').html(),
                                '2': $('<div>').text(ipport).html(),
                                '3': $('<div>').text(item.gen !== undefined ? String(item.gen) : '').html(),
                                '4': $('<div>').text(item.model || '').html(),
                                '5': $('<div>').text(item.fw || '').html(),
                                '6': flags
                            }).draw();
                        });
                    }
                    $('#modal').hide();
                },
                error: function () {
                    $('#modal').hide();
                    ShowNotify($.t('Problem getting nodes!'), 2500, true);
                }
            });

            $('#shellynodestable tbody').off('click.shellyrow').on('click.shellyrow', 'tr', function () {
                var deleteBtn2 = $('#shellyupdelclr #shellynodedelete');
                deleteBtn2.attr('class', 'btnstyle3-dis');
                deleteBtn2.removeAttr('data-nodeid');
                ShellySetSelectedNode(null);
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#hardwarecontent #shellynodeparamstable #shellynodename').val('');
                    $('#hardwarecontent #shellynodeparamstable #shellynodeip').val('');
                    $('#hardwarecontent #shellynodeparamstable #shellynodeport').val('');
                } else {
                    var oTable2 = $('#shellynodestable').DataTable();
                    oTable2.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    var anSelected = fnGetSelected(oTable2);
                    if (anSelected.length !== 0) {
                        var rowData = oTable2.row(anSelected[0]).data();
                        var nodeid = rowData['DT_RowId'];
                        deleteBtn2.attr('class', 'btnstyle3');
                        deleteBtn2.attr('data-nodeid', String(nodeid));
                        var addr = nodeAddresses[String(nodeid)] || { ip: '', port: '' };
                        var selName = $('<div>').html(rowData['1']).text();
                        $('#hardwarecontent #shellynodeparamstable #shellynodename').val(selName);
                        $('#hardwarecontent #shellynodeparamstable #shellynodeip').val(addr.ip);
                        $('#hardwarecontent #shellynodeparamstable #shellynodeport').val(addr.port);
                        ShellySetSelectedNode({ id: String(nodeid), name: selName, ip: addr.ip, port: addr.port });
                    }
                }
            });
        };

        ShellyToggleScan = function () {
            if (scanPollTimer) {
                ShellyStopScan();
            } else {
                ShellyStartScan();
            }
        };

        ShellyStartScan = function () {
            var range = $('#hardwarecontent #shellyscanparamstable #shellyscanrange').val().trim();
            if (range === '') {
                ShowNotify($.t('Please enter a scan range!'), 2500, true);
                return;
            }

            $.ajax({
                url: 'json.htm?type=command&param=shellyscan&idx=' + $.devIdx + '&range=' + encodeURIComponent(range),
                async: true,
                dataType: 'json',
                success: function (data) {
                    if (data.status === 'ERR') {
                        if (data.message) {
                            ShowNotify($('<span>').text(data.message).html(), 2500, true);
                        } else {
                            ShowNotify($.t('Problem starting scan!'), 2500, true);
                        }
                        return;
                    }
                    if (destroyed) {
                        return;
                    }
                    lastScanResultKey = '';
                    editedScanNames = {};
                    $('#shellyscanresultsbody').empty();
                    $('#shellyscanresults').show();
                    $('#shellyscanprogress').show();
                    $('#shellyscanstartstop').text($.t('Cancel Scan'));

                    scanPollTimer = setInterval(function () {
                        ShellyPollScanStatus();
                    }, 1000);
                },
                error: function () {
                    ShowNotify($.t('Problem starting scan!'), 2500, true);
                }
            });
        };

        ShellyStopScan = function () {
            if (scanPollTimer) {
                clearInterval(scanPollTimer);
                scanPollTimer = null;
            }
            $.ajax({
                url: 'json.htm?type=command&param=shellyscancancel&idx=' + $.devIdx,
                async: true,
                dataType: 'json'
            });
            $('#shellyscanstartstop').text($.t('Start Scan'));
            $('#shellyscanprogress').hide();
        };

        ShellyPollScanStatus = function () {
            if (scanPollInFlight) {
                return;
            }
            scanPollInFlight = true;
            $.ajax({
                url: 'json.htm?type=command&param=shellyscanstatus&idx=' + $.devIdx,
                async: true,
                dataType: 'json',
                success: function (data) {
                    scanPollInFlight = false;
                    var state = data.state || 'idle';
                    var probed = data.probed || 0;
                    var total  = data.total  || 0;
                    var found  = data.found  || 0;

                    if (total > 0) {
                        $('#shellyscanprogresstext').text(
                            ' ' + probed + ' / ' + total + ' ' + $.t('probed') + ', ' + found + ' ' + $.t('found')
                        );
                    }

                    ShellyUpdateScanResults(data.result || []);

                    if (state !== 'running') {
                        if (scanPollTimer) {
                            clearInterval(scanPollTimer);
                            scanPollTimer = null;
                        }
                        $('#shellyscanstartstop').text($.t('Start Scan'));
                        $('#shellyscanprogress').hide();
                    }
                },
                error: function () {
                    scanPollInFlight = false;
                    if (scanPollTimer) {
                        clearInterval(scanPollTimer);
                        scanPollTimer = null;
                    }
                    $('#shellyscanstartstop').text($.t('Start Scan'));
                    $('#shellyscanprogress').hide();
                    ShowNotify($.t('Problem polling scan status!'), 2500, true);
                }
            });
        };

        ShellyUpdateScanResults = function (results) {
            // Skip re-render when results are unchanged, preserving focus
            var newKey = JSON.stringify(results.map(function (r) {
                return [r.ip || '', r.port, r.name || '', r.alreadyAdded, r.ipChanged];
            }));
            if (newKey === lastScanResultKey) {
                return;
            }
            lastScanResultKey = newKey;

            var tbody = $('#shellyscanresultsbody');
            var focused = document.activeElement;
            var focusKey = (focused && $(focused).hasClass('shelly-scan-name')) ? $(focused).attr('data-key') : null;
            var caret = focusKey !== null && typeof focused.selectionStart === 'number' ? focused.selectionStart : null;
            tbody.empty();
            $.each(results, function (i, item) {
                var ip = item.ip || '';
                var ipport = ip + (item.port && item.port !== 80 ? ':' + item.port : '');
                var flags = '';
                if (item.authRequired) {
                    flags += '<span class="label label-warning">' + $.t('Auth Required') + '</span> ';
                }
                if (item.alreadyAdded) {
                    flags += '<span class="label label-success">' + $.t('Added') + '</span> ';
                }
                if (item.ipChanged) {
                    flags += '<span class="label label-important">' + $.t('IP changed') + '</span>';
                }

                var addBtnCell = $('<td>');
                var nameCell = $('<td>');
                var nameKey = ip + ':' + String(item.port || '');
                if (item.alreadyAdded) {
                    nameCell.text(item.name || '');
                } else {
                    // The name is editable before the device is added; an edit survives a re-render of the list
                    var deviceName = Object.prototype.hasOwnProperty.call(editedScanNames, nameKey) ? editedScanNames[nameKey] : (item.name || '');
                    var nameInput = $('<input type="text">')
                        .addClass('shelly-scan-name text ui-widget-content ui-corner-all')
                        .css({ width: '170px', 'max-width': '100%', padding: '.2em' })
                        .attr('maxlength', '64')
                        .attr('data-key', nameKey)
                        .attr('aria-label', $.t('Name') + ' ' + ipport)
                        .val(deviceName);
                    nameCell.append(nameInput);
                    var addBtn = $('<a>')
                        .addClass('btnstyle3 shelly-add-btn')
                        .attr('role', 'button')
                        .attr('tabindex', '0')
                        .attr('data-ip', ip)
                        .attr('data-port', String(item.port || ''))
                        .attr('aria-label', $.t('Add') + ' ' + ipport)
                        .text($.t('Add'));
                    addBtnCell.append(addBtn);
                }

                var row = $('<tr>');
                row.append($('<td>').text(ipport));
                row.append($('<td>').text(String(item.gen !== undefined ? item.gen : '')));
                row.append($('<td>').text(item.model || ''));
                row.append(nameCell);
                row.append($('<td>').text(item.fw || ''));
                row.append($('<td>').html(flags));
                row.append(addBtnCell);
                tbody.append(row);
            });
            if (focusKey !== null) {
                var again = tbody.find('input.shelly-scan-name').filter(function () { return $(this).attr('data-key') === focusKey; }).first();
                if (again.length) {
                    again.trigger('focus');
                    if (caret !== null && again[0].setSelectionRange) {
                        again[0].setSelectionRange(caret, caret);
                    }
                }
            }
        };

        ShellyAddFromScan = function (ip, port, name) {
            var url = 'json.htm?type=command&param=shellyaddnode&idx=' + $.devIdx + '&ip=' + encodeURIComponent(ip);
            if (port !== '') {
                url += '&port=' + encodeURIComponent(port);
            }
            if (name !== '') {
                url += '&name=' + encodeURIComponent(name);
            }

            $.ajax({
                url: url,
                async: true,
                dataType: 'json',
                success: function (data) {
                    if (data.status === 'ERR') {
                        ShellyShowAddError(data.message);
                    } else {
                        ShellyRefreshNodeTable();
                        ShellyPollScanStatus();
                    }
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        };
    }
});
