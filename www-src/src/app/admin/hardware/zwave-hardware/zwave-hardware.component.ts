import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {Hardware, ZWaveNode, ZWaveNodeConfig} from 'src/app/_shared/_models/hardware';
import {HardwareService} from '../../../_shared/_services/hardware.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Utils} from 'src/app/_shared/_utils/utils';
import {DatatableHelper} from '../../../_shared/_utils/datatable-helper';
import {ZwaveService} from 'src/app/admin/hardware/zwave.service';
import {interval, Subscription} from 'rxjs';
import {ZWaveExcludedNodeResponse, ZWaveGroupInfoResponse, ZWaveIncludedNodeResponse} from '../zwave';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {Router} from '@angular/router';
import {ZWaveIncludeModalComponent} from '../z-wave-include-modal/z-wave-include-modal.component';
import {ZWaveExcludeModalComponent} from '../z-wave-exclude-modal/z-wave-exclude-modal.component';
import * as Highcharts from 'highcharts';
import {Base64} from 'js-base64';
import {ConfigService} from '../../../_shared/_services/config.service';
import {ChartService} from '../../../_shared/_services/chart.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-zwave-hardware',
  templateUrl: './zwave-hardware.component.html',
  styleUrls: ['./zwave-hardware.component.css']
})
export class ZwaveHardwareComponent implements OnInit, AfterViewInit {

  @Input() hardware: Hardware;

  @ViewChild(ZWaveIncludeModalComponent, {static: false}) includeModal: ZWaveIncludeModalComponent;
  @ViewChild(ZWaveExcludeModalComponent, {static: false}) excludeModal: ZWaveExcludeModalComponent;
  // @ViewChild(ZwaveTopologyModalComponent) topologyModal: ZwaveTopologyModalComponent;

  displayusercodegrp = false;

  nodename = '';
  displayzwavenodesqueried: boolean;
  ownNodeId: string;
  selectedData: TableItem;
  displaytrEnablePolling = false;

  mytimer: Subscription;
  EnablePolling: boolean;

  @ViewChild('networkChart', {static: false}) networkChartRef: ElementRef;
  depChart: Highcharts.Chart;

  nodeupdateenabled = false;
  nodedeleteenabled = false;
  noderefreshenabled = false;

  constructor(
    private hardwareService: HardwareService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private zwaveService: ZwaveService,
    private notificationService: NotificationService,
    private router: Router,
    private configService: ConfigService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.EditOpenZWave();
  }

  private EditOpenZWave() {
    this.displayusercodegrp = false;

    $('#nodestable').dataTable({
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
      language: this.configService.dataTableDefaultSettings.language
    });

    this.RefreshOpenZWaveNodeTable();
  }

  RefreshOpenZWaveNodeTable() {
    this.nodeupdateenabled = false;
    this.nodedeleteenabled = false;
    this.noderefreshenabled = false;
    this.selectedData = undefined;

    this.nodename = '';

    const oTable = $('#nodestable').dataTable();
    oTable.fnClearTable();

    this.hardwareService.getOpenZWaveNodes(this.hardware.idx.toString()).subscribe((response) => {
      if (typeof response.result !== 'undefined') {

        if (response.NodesQueried === true) {
          this.displayzwavenodesqueried = false;
        } else {
          this.displayzwavenodesqueried = true;
        }

        this.ownNodeId = String(response.ownNodeId);

        response.result.forEach((item: ZWaveNode) => {
          let status = 'ok';
          if (item.State === 'Dead') {
            status = 'failed';
          } else if ((item.State === 'Sleep') || (item.State === 'Sleeping')) {
            status = 'sleep';
          } else if (item.State === 'Unknown') {
            status = 'unknown';
          }

          const statusImg = '<img src="images/' + status + '.png" />';
          const healButton = '<img src="images/heal.png" class="lcursor zwavehealnode-click" nodeid="' + item.NodeID + '" title="' +
            this.translationService.t('Heal node') + '" />';

          let Description = item.Description;
          if (Description.length < 2) {
            Description = '<span class="zwave_no_info">' + item.Generic_type + '</span>';
          }
          if (item.IsPlus === true) {
            Description += '+';
          }

          const nodeStr = Utils.addLeadingZeros(item.NodeID, 3) + ' (0x' + Utils.addLeadingZeros(item.NodeID.toString(16), 2) + ')';

          oTable.fnAddData(<TableItem>{
            'DT_RowId': item.idx,
            'Name': item.Name,
            'PollEnabled': item.PollEnabled,
            'Config': item.config,
            'State': item.State,
            'NodeID': item.NodeID,
            'HaveUserCodes': item.HaveUserCodes,
            '0': nodeStr,
            '1': item.Name,
            '2': Description,
            '3': item.Manufacturer_name,
            '4': item.Product_id,
            '5': item.Product_type,
            '6': item.LastUpdate,
            '7': this.translationService.t((item.PollEnabled === 'true') ? 'Yes' : 'No'),
            '8': item.Battery,
            '9': statusImg + '&nbsp;&nbsp;' + healButton
          });
        });

        const _this = this;

        $('.zwavehealnode-click').off().on('click', function (e) {
          e.preventDefault();
          _this.ZWaveHealNode(this.nodeid);
        });

        /* Add a click handler to the rows - this could be used as a callback */
        $('#nodestable tbody').off();
        $('#nodestable tbody').on('click', 'tr', function () {
          if ($(this).hasClass('row_selected')) {
            $(this).removeClass('row_selected');
            _this.nodeupdateenabled = false;
            _this.nodedeleteenabled = false;
            _this.noderefreshenabled = false;
            _this.selectedData = undefined;
            _this.nodename = '';
            _this.displayusercodegrp = false;
          } else {
            const iOwnNodeId = Number(_this.ownNodeId);
            //  const oTable = $('#nodestable').dataTable();
            oTable.$('tr.row_selected').removeClass('row_selected');
            $(this).addClass('row_selected');
            _this.nodeupdateenabled = true;
            _this.noderefreshenabled = true;
            const anSelected = DatatableHelper.fnGetSelected(oTable);
            if (anSelected.length !== 0) {
              const data = oTable.fnGetData(anSelected[0]) as TableItem;
              _this.selectedData = data;
              const iNode = data['NodeID'];
              if (iNode !== iOwnNodeId) {
                _this.nodedeleteenabled = true;
              }
              _this.nodename = data['Name'];
              _this.displaytrEnablePolling = iNode !== iOwnNodeId;
              _this.EnablePolling = data.PollEnabled === 'true';
              _this.displayusercodegrp = !data['HaveUserCodes'];
            }
          }
        });

        this.RefreshGroupTable();
        this.RefreshNeighbours();
      }
    });
  }

  ZWaveHealNode(node: string) {
    this.zwaveService.heal(this.hardware.idx.toString(), node).subscribe(() => {
      bootbox.alert(this.translationService.t('Initiating node heal...!'));
    });
  }

  ZWaveIncludeNode(isSecure: boolean) {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    this.includeModal.displayizwd_waiting = true;
    this.includeModal.displayizwd_result = false;
    this.zwaveService.include(this.hardware.idx.toString(), isSecure).subscribe(() => {
      this.includeModal.ozw_node_id = '-';
      this.includeModal.ozw_node_desc = '-';
      this.includeModal.open();
      this.mytimer = interval(1000).subscribe(() => {
        this.ZWaveCheckIncludeReady();
      });
    });
  }

  ZWaveCheckIncludeReady() {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    this.zwaveService.isIncluded(this.hardware.idx.toString()).subscribe((data: ZWaveIncludedNodeResponse) => {
      if (data.status === 'OK') {
        if (data.result === true) {
          // Node included
          this.includeModal.ozw_node_id = data.node_id;
          this.includeModal.ozw_node_desc = data.node_product_name;
          this.includeModal.displayizwd_waiting = false;
          this.includeModal.displayizwd_result = true;
        } else {
          // Not ready yet
          this.mytimer = interval(1000).subscribe(() => {
            this.ZWaveCheckIncludeReady();
          });
        }
      } else {
        this.mytimer = interval(1000).subscribe(() => {
          this.ZWaveCheckIncludeReady();
        });
      }
    }, () => {
      this.mytimer = interval(1000).subscribe(() => {
        this.ZWaveCheckIncludeReady();
      });
    });
  }

  ZWaveExcludeNode() {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    this.excludeModal.displayezwd_waiting = true;
    this.excludeModal.displayezwd_result = false;
    this.zwaveService.exclude(this.hardware.idx.toString()).subscribe((data: ZWaveExcludedNodeResponse) => {
      this.excludeModal.ozw_node_id = data.node_id;
      this.excludeModal.open();
      this.mytimer = interval(1000).subscribe(() => {
        this.ZWaveCheckExcludeReady();
      });
    });
  }

  ZWaveCheckExcludeReady() {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    this.zwaveService.isExcluded(this.hardware.idx.toString()).subscribe((data: ZWaveExcludedNodeResponse) => {
      if (data.status === 'OK') {
        if (data.result === true) {
          // Node excluded
          this.excludeModal.ozw_node_id = data.node_id;
          this.excludeModal.displayezwd_waiting = false;
          this.excludeModal.displayezwd_result = true;
        } else {
          // Not ready yet
          this.mytimer = interval(1000).subscribe(() => {
            this.ZWaveCheckExcludeReady();
          });
        }
      } else {
        this.mytimer = interval(1000).subscribe(() => {
          this.ZWaveCheckExcludeReady();
        });
      }
    }, () => {
      this.mytimer = interval(1000).subscribe(() => {
        this.ZWaveCheckExcludeReady();
      });
    });
  }

  ZWaveSoftResetNode() {
    this.zwaveService.softReset(this.hardware.idx.toString()).subscribe(() => {
      bootbox.alert(this.translationService.t('Soft resetting controller device...!'));
    });
  }

  ZWaveHardResetNode() {
    bootbox.confirm(
      this.translationService.t('Are you sure you want to hard reset the controller?\n(All associated nodes will be removed)'),
      (result) => {
        if (result === true) {
          this.zwaveService.hardReset(this.hardware.idx.toString()).subscribe(() => {
            bootbox.alert(this.translationService.t('Hard resetting controller device...!'));
          });
        }
      });
  }

  ZWaveReceiveConfiguration() {
    this.zwaveService.receiveConfigurationFromOtherController(this.hardware.idx.toString()).subscribe(() => {
      bootbox.alert(this.translationService.t('Initiating Receive Configuration...!'));
    });
  }

  ZWaveSendConfiguration() {
    this.zwaveService.sendConfigurationToSecondController(this.hardware.idx.toString()).subscribe(() => {
      bootbox.alert(this.translationService.t('Initiating Send Configuration...!'));
    });
  }

  ZWaveTransferPrimaryRole() {
    bootbox.confirm(this.translationService.t('Are you sure you want to transfer the primary role?'), (result) => {
      if (result === true) {
        this.zwaveService.transferPrimaryRole(this.hardware.idx.toString()).subscribe(() => {
          bootbox.alert(this.translationService.t('Initiating Transfer Primary Role...!'));
        });
      }
    });
  }

  ZWaveStartUserCodeEnrollment() {
    this.zwaveService.startUserCodeEnrollment(this.hardware.idx.toString()).subscribe(() => {
      bootbox.alert(this.translationService.t('User Code Enrollment started. You have 30 seconds to include the new key...!'));
    });
  }

  ZWaveUserCodeManagement(idx: string) {
    this.router.navigate(['/Hardware', this.hardware.idx.toString(), 'ZWaveUserCode', idx]);
  }

  UpdateNode(idx: string) {
    if (this.nodeupdateenabled === false) {
      return;
    }
    const name = this.nodename;
    if (name === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
      return;
    }

    const bEnablePolling = this.EnablePolling;
    this.zwaveService.updateNode(idx, name, bEnablePolling).subscribe(() => {
      this.RefreshOpenZWaveNodeTable();
    }, () => {
      this.notificationService.ShowNotify(this.translationService.t('Problem updating Node!'), 2500, true);
    });
  }

  DeleteNode(idx: string) {
    if (this.nodedeleteenabled === false) {
      return;
    }
    bootbox.confirm(this.translationService.t('Are you sure to remove this Node?'), (result) => {
      if (result === true) {
        this.zwaveService.deleteNode(idx).subscribe(() => {
          bootbox.alert(this.translationService.t('Node marked for Delete. This could take some time!'));
          this.RefreshOpenZWaveNodeTable();
        });
      }
    });
  }

  // Request Node Information Frame
  RefreshNode(idx: string) {
    if (this.noderefreshenabled === false) {
      return;
    }
    this.zwaveService.requestNodeInfo(idx).subscribe(() => {
      bootbox.alert(
        this.translationService.t('Node Information Frame requested. This could take some time! (You might need to wake-up the node!)'));
    });
  }

  RequestZWaveConfiguration(idx: string) {
    this.zwaveService.requestNodeConfig(idx).subscribe(() => {
      bootbox.alert(this.translationService.t('Configuration requested from Node. If the Node is asleep, this could take a while!'));
      this.RefreshOpenZWaveNodeTable();
    }, () => {
      this.notificationService.ShowNotify(this.translationService.t('Problem requesting Node Configuration!'), 2500, true);
    });
  }

  ApplyZWaveConfiguration(idx: string) {
    let valueList = '';
    let $list = $('#hardwarecontent #configuration input');
    if (typeof $list !== 'undefined') {
      // Now loop through list of controls

      $list.each(function () {
        const id = $(this).prop('id');      // get id
        const value = Base64.encode($(this).prop('value'));      // get value

        valueList += id + '_' + value + '_';
      });
    }

    // Now for the Lists
    $list = $('#hardwarecontent #configuration select');
    if (typeof $list !== 'undefined') {
      // Now loop through list of controls
      $list.each(function () {
        const id = $(this).prop('id');      // get id
        const value = Base64.encode($(this).find(':selected').text());      // get value
        valueList += id + '_' + value + '_';
      });
    }

    if (valueList !== '') {
      this.zwaveService.applyNodeConfig(idx, valueList).subscribe(() => {
        bootbox.alert(this.translationService.t('Configuration sent to node. If the node is asleep, this could take a while!'));
      }, () => {
        this.notificationService.ShowNotify(this.translationService.t('Problem updating Node Configuration!'), 2500, true);
      });
    }
  }

  ZWaveHealNetwork() {
    this.zwaveService.networkHeal(this.hardware.idx.toString()).subscribe(() => {
      // Nothing
    });
  }

  OnZWaveAbortInclude() {
    this.zwaveService.cancel(this.hardware.idx.toString()).subscribe(() => {
      this.includeModal.dismiss();
    }, () => {
      this.includeModal.dismiss();
    });
  }

  OnZWaveCloseInclude() {
    this.RefreshOpenZWaveNodeTable();
  }

  OnZWaveAbortExclude() {
    this.zwaveService.cancel(this.hardware.idx.toString()).subscribe(() => {
      this.excludeModal.dismiss();
    }, () => {
      this.excludeModal.dismiss();
    });
  }

  OnZWaveCloseExclude() {
    this.RefreshOpenZWaveNodeTable();
  }

  getNodeName(node) {
    return node ? node.nodeName : '';
  }

  getGroupName(group) {
    return group ? group.groupName : '';
  }

  removeNodeFromGroup(event) {
    const node = event.data.node;
    const group = event.data.group;
    const removeNode = event.data.removeNode;
    const removeNodeId = event.data.removeNodeId;
    bootbox.confirm(this.translationService.t(
      'Are you sure you want to remove this node?<br>'
      + 'Node: ' + this.getNodeName(removeNode)
      + ' (' + removeNodeId
      + ')<br>From group: ' + this.getGroupName(group)
      + '<br>On node ' + this.getNodeName(node)), (result) => {
      if (result === true) {
        this.zwaveService.removeGroupNode(this.hardware.idx.toString(), node.nodeID, group.id, removeNodeId).subscribe(() => {
          bootbox.alert(this.translationService.t('Groups updated!'));
          $('#grouptable').empty(); // FIXME do it with Angular
          this.RefreshGroupTable();
        }, () => {
          // Nothing
        });
      }
    });
  }

  addNodeToGroup(event) {
    const node = event.data.node;
    const group = event.data.group;
    const _this = this;
    bootbox.dialog({
      message: this.translationService.t('Please enter the node to add to this group:') +
        '<input type=\'text\' id=\'add_node\' data-toggle=\'tooltip\' title=\'NodeId or NodeId.Instance\'></input><br>' + group.groupName,
      title: 'Add node to ' + this.getNodeName(node),
      buttons: {
        main: {
          label: this.translationService.t('Save'),
          className: 'btn-primary',
          callback: function () {
            const addnode = $('#add_node').val();
            _this.zwaveService.addGroupNode(_this.hardware.idx.toString(), node.nodeID, group.id, addnode).subscribe(() => {
              _this.RefreshGroupTable();
            }, () => {
              // Nothing
            });
          }
        },
        nothanks: {
          label: this.translationService.t('Cancel'),
          className: 'btn-cancel',
          callback: function () {
          }
        }
      }
    });
  }

  RefreshGroupTable() {
    this.zwaveService.getGroupInfo(this.hardware.idx.toString()).subscribe((data: ZWaveGroupInfoResponse) => {
      if (typeof data !== 'undefined') {
        if (data.status === 'OK') {
          if (typeof data.result !== 'undefined') {

            if ($.fn.DataTable.isDataTable('#grouptable')) {
              const oTable = $('#grouptable').DataTable();
              oTable.destroy();
            }

            let i;
            $('#grouptable').empty();
            const grouptable = $('#grouptable');
            const maxgroups = data.result.MaxNoOfGroups;
            const thead = $('<thead></thead>');
            const trow = $('<tr style="height: 35px;"></tr>');

            $('<th></th>').text(this.translationService.t('Node')).appendTo(trow);
            $('<th></th>').text(this.translationService.t('Name')).appendTo(trow);
            for (i = 0; i < maxgroups; i++) {
              $('<th></th>').text(this.translationService.t('Group') + ' ' + (i + 1)).appendTo(trow);
            }
            trow.appendTo(thead);
            thead.appendTo(grouptable);
            const tbody = $('<tbody></tbody>');

            const jsonnodes = data.result.nodes;
            const nodes = {};
            i = jsonnodes.length;
            while (i--) {
              const node = jsonnodes[i];
              nodes[node.nodeID] = node;
            }
            jsonnodes.forEach((node, i) => {
              let groupsDone = 0;
              const noderow = $('<tr>');
              const nodeStr = Utils.addLeadingZeros(node.nodeID, 3) + ' (0x' + Utils.addLeadingZeros(node.nodeID.toString(16), 2) + ')';
              $('<td>').text(nodeStr).appendTo(noderow);
              $('<td>').text(node.nodeName).appendTo(noderow);
              const noGroups = node.groupCount;
              if (noGroups > 0) {
                const groupnodes = node.groups;
                groupnodes.forEach((group) => {
                  const td = $('<td>').data('toggle', 'tooltip').attr('title', this.getGroupName(group));

                  const createButton = (text) => {
                    const button2 = $('<span>');
                    button2.text(text);
                    button2.addClass('label label-info lcursor');
                    return button2;
                  };

                  group.nodes.split(',').forEach((t) => {
                    if (t !== '') {
                      const button = createButton(t);
                      const removeNodeId = Number(t) * 1;
                      const removeNode = nodes[Math.round(removeNodeId)];
                      button.click({
                        node: node,
                        group: group,
                        removeNode: removeNode,
                        removeNodeId: t
                      }, this.removeNodeFromGroup);
                      button.attr('title', this.getNodeName(removeNode) + ' on ' + this.getGroupName(group));
                      button.appendTo(td);
                    }
                  });
                  const button3 = createButton('+');
                  button3.click({node: node, group: group}, this.addNodeToGroup);
                  button3.appendTo(td);
                  td.appendTo(noderow);
                  groupsDone++;
                });
              }
              if (groupsDone < maxgroups) {
                const missing = maxgroups - noGroups;
                for (i = 0; i < missing; i++) {
                  $('<td></td>').text(' ').appendTo(noderow);
                  groupsDone++;
                }
              }
              noderow.appendTo(tbody);
            });
            tbody.appendTo(grouptable);
            const tfoot = $('<tfoot></tfoot>');
            tfoot.appendTo(grouptable);

            $('#grouptable').DataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
              'sDom': '<"H"lfrC>t<"F"ip>',
              'oTableTools': {
                'sRowSelect': 'single'
              },
              'aaSorting': [[0, 'desc']],
              'bSortClasses': false,
              'bProcessing': true,
              'bStateSave': true,
              'bJQueryUI': true,
              'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, 'All']],
              'iDisplayLength': 25,
              'sPaginationType': 'full_numbers',
              language: this.configService.dataTableDefaultSettings.language
            }));
          }
        }
      }
    }, () => {
      // Nothing
    });
  }

  RefreshNeighbours() {
    const _this = this;

    const chartOptions: Highcharts.Options = {
      chart: {
        type: 'spline',
        zoomType: 'x',
        resetZoomButton: {
          position: {
            x: -30,
            y: -36
          }
        },
        events: {
          load: function () {
            _this.zwaveService.getNetworkInfo(_this.hardware.idx.toString()).subscribe((data) => {
              if (typeof data !== 'undefined') {
                if (data.status === 'OK') {
                  if (typeof data.result !== 'undefined') {
                    const datachart = [];
                    data.result.mesh.forEach((item) => {
                      if (item.seesNodes) {
                        const seesNodes = item.seesNodes.split(',');
                        for (let sn = 0; sn < seesNodes.length; sn++) {
                          datachart.push({from: item.nodeID, to: seesNodes[sn], weight: 1});
                        }
                      } else {
                        // node is stand alone
                        datachart.push({from: item.nodeID, to: item.nodeID, weight: 1});
                      }
                    });
                    const series = _this.depChart.series[0];
                    series.setData(datachart);
                  }
                }
              }
            }, () => {
            });
          }
        }
      },
      title: {
        text: null
      },
      tooltip: {
        enabled: false
      },
      series: [{
        keys: ['from', 'to', 'weight'],
        type: 'dependencywheel',
        name: 'Node Neighbors',
        dataLabels: {
          color: '#fff',
          textPath: {
            enabled: true,
            attributes: {
              dy: 5
            }
          }
        },
        tooltip: {
          distance: 10
        },
        size: '95%'
      }]
    };

    ChartService.setDeviceIdx(this.hardware.idx.toString());
    this.depChart = Highcharts.chart(this.networkChartRef.nativeElement, chartOptions);
  }

}

interface TableItem {
  'DT_RowId': string;
  'Name': string;
  'PollEnabled': string;
  'Config': Array<ZWaveNodeConfig>;
  'State': string;
  'NodeID': number;
  'HaveUserCodes': boolean;
  '0': string;
  '1': string;
  '2': string;
  '3': string;
  '4': string;
  '5': string;
  '6': string;
  '7': string;
  '8': string;
}
