import {AfterViewInit, Component, Inject, Input, OnInit} from '@angular/core';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {BleBoxService} from 'src/app/admin/hardware/ble-box.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-ble-box-hardware',
  templateUrl: './ble-box-hardware.component.html',
  styleUrls: ['./ble-box-hardware.component.css']
})
export class BleBoxHardwareComponent implements OnInit, AfterViewInit {

  @Input() hardware: Hardware;

  devIdx: string;
  pollinterval: number;
  pingtimeout: number;
  ipmask: string;
  nodename = '';
  nodeip = '';
  nodeidx: any;

  constructor(
    private bleBoxService: BleBoxService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private configService: ConfigService
  ) {
  }

  ngOnInit() {
    this.devIdx = this.hardware.idx.toString();

    this.pollinterval = this.hardware.Mode1;
    this.pingtimeout = this.hardware.Mode2;
    this.ipmask = '192.168.1.*';
  }

  ngAfterViewInit() {
    $('#bleboxnodestable').dataTable({
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

    this.RefreshBleBoxNodeTable();
  }

  RefreshBleBoxNodeTable() {
    // $('#modal').show();
    $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
    this.nodename = '';
    this.nodeip = '';

    const oTable = $('#bleboxnodestable').dataTable();
    oTable.fnClearTable();

    this.bleBoxService.getNodes(this.devIdx).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item) => {
          oTable.fnAddData({
            'DT_RowId': item.idx,
            'Name': item.Name,
            'IP': item.IP,
            '0': item.idx,
            '1': item.Name,
            '2': item.IP,
            '3': item.Type,
            '4': item.Uptime,
            '5': item.hv,
            '6': item.fv
          });
        });
      }
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#bleboxnodestable tbody').off();
    $('#bleboxnodestable tbody').on('click', 'tr', function () {
      $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        _this.nodename = '';
        _this.nodeip = '';
      } else {
        // var oTable = $('#bleboxnodestable').dataTable();
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const idx = data['DT_RowId'];
          _this.nodeidx = idx;
          $('#updelclr #nodedelete').attr('class', 'btnstyle3');
          _this.nodename = data['1'];
          _this.nodeip = data['2'];
        }
      }
    });

    $('#modal').hide();
  }

  SetBleBoxSettings() {
    let Mode1 = Number(this.pollinterval);
    if (Mode1 < 1) {
      Mode1 = 30;
    }
    let Mode2 = Number(this.pingtimeout);
    if (Mode2 < 1000) {
      Mode2 = 1000;
    }

    this.bleBoxService.setMode(this.devIdx, Mode1, Mode2).subscribe(() => {
      bootbox.alert(this.translationService.t('Settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Settings!'), 2500, true);
    });
  }

  BleBoxAutoSearchingNodes() {
    const ipmask = this.ipmask;
    if (ipmask === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a mask!'), 2500, true);
      return;
    }

    this.bleBoxService.autoSearchingNodes(this.devIdx, ipmask).subscribe(() => {
      this.RefreshBleBoxNodeTable();
    });
  }

  BleBoxUpdateFirmware() {
    this.notificationService.ShowNotify(this.translationService.t('Please wait. Updating ....!'), 2500, true);

    this.bleBoxService.updateFirmware(this.devIdx).subscribe(() => {
      this.RefreshBleBoxNodeTable();
    });
  }

  BleBoxClearNodes() {
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Nodes?\n\nThis action can not be undone!'), (result) => {
      if (result === true) {
        this.bleBoxService.clearNodes(this.devIdx).subscribe(() => {
          this.RefreshBleBoxNodeTable();
        });
      }
    });
  }

  BleBoxAddNode() {
    const name = this.nodename;
    if (name === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
      return;
    }
    const ip = this.nodeip;
    if (ip === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a IP Address!'), 2500, true);
      return;
    }

    this.bleBoxService.addNode(this.devIdx, name, ip).subscribe(() => {
      this.RefreshBleBoxNodeTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Adding Node!'), 2500, true);
    });
  }

  BleBoxDeleteNode(nodeid: any) {
    if ($('#updelclr #nodedelete').attr('class') === 'btnstyle3-dis') {
      return;
    }
    bootbox.confirm(this.translationService.t('Are you sure to remove this Node?'), (result) => {
      if (result === true) {
        this.bleBoxService.removeNode(this.devIdx, nodeid.toString()).subscribe(() => {
          this.RefreshBleBoxNodeTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem Deleting Node!'), 2500, true);
        });
      }
    });
  }

}
