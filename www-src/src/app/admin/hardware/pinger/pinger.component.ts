import {AfterViewInit, Component, Inject, Input, OnInit} from '@angular/core';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {PingerService} from '../pinger.service';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-pinger',
  templateUrl: './pinger.component.html',
  styleUrls: ['./pinger.component.css']
})
export class PingerComponent implements OnInit, AfterViewInit {

  @Input() hardware: Hardware;

  devIdx: string;
  pollinterval: number;
  pingtimeout: number;
  nodename = '';
  nodeip = '';
  nodetimeout = '5';
  nodeidx: any;

  constructor(
    private pingerService: PingerService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
    this.devIdx = this.hardware.idx.toString();

    this.pollinterval = this.hardware.Mode1;
    this.pingtimeout = this.hardware.Mode2;
  }

  ngAfterViewInit() {
    $('#ipnodestable').dataTable({
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

    this.RefreshPingerNodeTable();
  }

  RefreshPingerNodeTable() {
    // $('#modal').show();
    $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
    $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
    this.nodename = '';
    this.nodeip = '';
    this.nodetimeout = '5';

    const oTable = $('#ipnodestable').dataTable();
    oTable.fnClearTable();

    this.pingerService.getNodes(this.devIdx).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item) => {
          oTable.fnAddData({
            'DT_RowId': item.idx,
            'Name': item.Name,
            'IP': item.IP,
            '0': item.idx,
            '1': item.Name,
            '2': item.IP,
            '3': item.Timeout
          });
        });
      }
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#ipnodestable tbody').off();
    $('#ipnodestable tbody').on('click', 'tr', function () {
      $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
        _this.nodename = '';
        _this.nodeip = '';
        _this.nodetimeout = '5';
      } else {
        // var oTable = $('#ipnodestable').dataTable();
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        $('#updelclr #nodeupdate').attr('class', 'btnstyle3');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const idx = data['DT_RowId'];
          _this.nodeidx = idx;
          $('#updelclr #nodedelete').attr('class', 'btnstyle3');
          _this.nodename = data['1'];
          _this.nodeip = data['2'];
          _this.nodetimeout = data['2'];
        }
      }
    });

    // $('#modal').hide();
  }

  SetPingerSettings() {
    let Mode1 = Number(this.pollinterval);
    if (Mode1 < 1) {
      Mode1 = 30;
    }
    let Mode2 = Number(this.pingtimeout);
    if (Mode2 < 500) {
      Mode2 = 500;
    }
    this.pingerService.setMode(this.devIdx, Mode1, Mode2).subscribe(() => {
      bootbox.alert(this.translationService.t('Settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Settings!'), 2500, true);
    });
  }

  PingerClearNodes() {
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Nodes?\n\nThis action can not be undone!'), (result) => {
      if (result === true) {
        this.pingerService.clearNodes(this.devIdx).subscribe(() => {
          this.RefreshPingerNodeTable();
        });
      }
    });
  }

  AddPingerNode() {
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
    let Timeout = Number(this.nodetimeout);
    if (Timeout < 1) {
      Timeout = 5;
    }

    this.pingerService.addNode(this.devIdx, name, ip, Timeout).subscribe(() => {
      this.RefreshPingerNodeTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Adding Node!'), 2500, true);
    });
  }

  PingerUpdateNode(nodeid: any) {
    if ($('#updelclr #nodedelete').attr('class') === 'btnstyle3-dis') {
      return;
    }

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
    let Timeout = Number(this.nodetimeout);
    if (Timeout < 1) {
      Timeout = 5;
    }

    this.pingerService.updateNode(this.devIdx, nodeid, name, ip, Timeout).subscribe(() => {
      this.RefreshPingerNodeTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Node!'), 2500, true);
    });
  }

  PingerDeleteNode(nodeid: any) {
    if ($('#updelclr #nodedelete').attr('class') === 'btnstyle3-dis') {
      return;
    }
    bootbox.confirm(this.translationService.t('Are you sure to remove this Node?'), (result) => {
      if (result === true) {
        this.pingerService.removeNode(this.devIdx, nodeid).subscribe(() => {
          this.RefreshPingerNodeTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem Deleting Node!'), 2500, true);
        });
      }
    });
  }

}
