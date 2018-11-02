import {AfterViewInit, Component, Inject, Input, OnInit} from '@angular/core';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {KodiService} from 'src/app/admin/hardware/kodi.service';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-kodi',
  templateUrl: './kodi.component.html',
  styleUrls: ['./kodi.component.css']
})
export class KodiComponent implements OnInit, AfterViewInit {

  @Input() hardware: Hardware;

  devIdx: string;
  pollinterval: number;
  pingtimeout: number;
  nodename = '';
  nodeip = '';
  nodeport = '9090';
  nodeidx: any;

  constructor(
    private kodiService: KodiService,
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
    $('#kodinodestable').dataTable({
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

    this.RefreshKodiNodeTable();
  }

  RefreshKodiNodeTable() {
    // $('#modal').show();
    $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
    $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
    this.nodename = '';
    this.nodeip = '';
    this.nodeport = '9090';

    const oTable = $('#kodinodestable').dataTable();
    oTable.fnClearTable();

    this.kodiService.getNodes(this.devIdx).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item) => {
          oTable.fnAddData({
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
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#kodinodestable tbody').off();
    $('#kodinodestable tbody').on('click', 'tr', function () {
      $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
        _this.nodename = '';
        _this.nodeip = '';
        _this.nodeip = '9090';
      } else {
        // var oTable = $('#kodinodestable').dataTable();
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
          _this.nodeport = data['3'];
        }
      }
    });

    // $('#modal').hide();
  }

  SetKodiSettings() {
    let Mode1 = Number(this.pollinterval);
    if (Mode1 < 1) {
      Mode1 = 30;
    }
    let Mode2 = Number(this.pingtimeout);
    if (Mode2 < 500) {
      Mode2 = 500;
    }
    this.kodiService.setMode(this.devIdx, Mode1, Mode2).subscribe(() => {
      bootbox.alert(this.translationService.t('Settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Settings!'), 2500, true);
    });
  }

  KodiClearNodes() {
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Nodes?\n\nThis action can not be undone!'), (result) => {
      if (result === true) {
        this.kodiService.clearNodes(this.devIdx).subscribe(() => {
          this.RefreshKodiNodeTable();
        });
      }
    });
  }

  AddKodiNode() {
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
    if (this.nodeport === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Port!'), 2500, true);
      return;
    }
    const Port = Number(this.nodeport);

    this.kodiService.addNode(this.devIdx, name, ip, Port).subscribe(() => {
      this.RefreshKodiNodeTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Adding Node!'), 2500, true);
    });
  }

  KodiUpdateNode(nodeid: any) {
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
    if (this.nodeport === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Port!'), 2500, true);
      return;
    }
    const Port = Number(this.nodeport);

    this.kodiService.updateNode(this.devIdx, nodeid.toString(), name, ip, Port).subscribe(() => {
      this.RefreshKodiNodeTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Node!'), 2500, true);
    });
  }

  KodiDeleteNode(nodeid: any) {
    if ($('#updelclr #nodedelete').attr('class') === 'btnstyle3-dis') {
      return;
    }
    bootbox.confirm(this.translationService.t('Are you sure to remove this Node?'), (result) => {
      if (result === true) {
        this.kodiService.removeNode(this.devIdx, nodeid.toString()).subscribe(() => {
          this.RefreshKodiNodeTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem Deleting Node!'), 2500, true);
        });
      }
    });
  }

}
