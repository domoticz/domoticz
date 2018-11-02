import {AfterViewInit, Component, Inject, Input, OnInit} from '@angular/core';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {MySensorsService} from 'src/app/admin/hardware/my-sensors.service';
import {MySensor} from 'src/app/admin/hardware/my-sensors';
import {MySensorChild} from '../my-sensors';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-my-sensors',
  templateUrl: './my-sensors.component.html',
  styleUrls: ['./my-sensors.component.css']
})
export class MySensorsComponent implements OnInit, AfterViewInit {

  @Input() hardware: Hardware;

  devIdx: string;
  nodeid: number;
  nodename: string;

  displaytrChildSettings = false;
  displaytrNodeSettings = false;
  Ack: boolean;
  AckTimeout: any;
  childidx: any;

  constructor(
    private mySensorsService: MySensorsService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
    this.devIdx = this.hardware.idx.toString();
  }

  ngAfterViewInit() {
    $('#mysensorsnodestable').dataTable({
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single'
      },
      'aaSorting': [[0, 'asc']],
      'bSortClasses': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, 'All']],
      'iDisplayLength': 25,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });
    $('#mysensorsactivetable').dataTable({
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single'
      },
      'aaSorting': [[0, 'asc']],
      'bSortClasses': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, 'All']],
      'iDisplayLength': 25,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });
    this.RefreshMySensorsNodeTable();
  }

  RefreshMySensorsNodeTable() {
    this.displaytrChildSettings = false;
    this.displaytrNodeSettings = false;
    // $('#modal').show();

    const oTable = $('#mysensorsnodestable').dataTable();
    oTable.fnClearTable();

    this.nodeid = -1;

    this.mySensorsService.getNodes(this.devIdx).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item: MySensor) => {
          oTable.fnAddData({
            'DT_RowId': item.idx,
            'Name': item.Name,
            'SketchName': item.SketchName,
            'Version': item.Version,
            '0': item.idx,
            '1': item.Name,
            '2': item.SketchName,
            '3': item.Version,
            '4': item.Childs,
            '5': item.LastReceived
          });
        });
      }
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#mysensorsnodestable tbody').off();
    $('#mysensorsnodestable tbody').on('click', 'tr', function () {
      $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
      $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
      _this.displaytrNodeSettings = false;
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
      } else {
        // var oTable = $('#mysensorsnodestable').dataTable();
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const idx = data['DT_RowId'];
          $('#updelclr #nodedelete').attr('class', 'btnstyle3');
          $('#updelclr #nodeupdate').attr('class', 'btnstyle3');
          _this.nodename = data['Name'];
          _this.displaytrNodeSettings = true;
          _this.nodeid = idx;
          _this.MySensorsRefreshActiveDevicesTable();
        }
      }
    });

    // $('#modal').hide();
  }

  MySensorsRefreshActiveDevicesTable() {
    // $('#plancontent #delclractive #activedevicedelete').attr("class", "btnstyle3-dis");
    $('#hardwarecontent #trChildSettings').hide();
    const oTable = $('#mysensorsactivetable').dataTable();
    oTable.fnClearTable();
    if (this.nodeid === -1) {
      return;
    }

    this.mySensorsService.getChildren(this.devIdx, this.nodeid).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        // var totalItems = data.result.length;
        data.result.forEach((item: MySensorChild) => {
          oTable.fnAddData({
            'DT_RowId': item.child_id,
            'AckEnabled': item.use_ack,
            'AckTimeout': item.ack_timeout,
            '0': item.child_id,
            '1': item.type,
            '2': item.name,
            '3': item.Values,
            '4': item.use_ack,
            '5': item.ack_timeout,
            '6': item.LastReceived
          });
        });
      }
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#mysensorsactivetable tbody').off();
    $('#mysensorsactivetable tbody').on('click', 'tr', function () {
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        $('#delclractive #activedevicedelete').attr('class', 'btnstyle3-dis');
        $('#delclractive #activedeviceupdate').attr('class', 'btnstyle3-dis');
        _this.displaytrChildSettings = false;
      } else {
        // var oTable = $('#mysensorsactivetable').dataTable();
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        $('#activedevicedelete').attr('class', 'btnstyle3');
        $('#activedeviceupdate').attr('class', 'btnstyle3');
        _this.displaytrChildSettings = true;
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const idx = data['DT_RowId'];
          _this.childidx = idx;
          _this.Ack = data['AckEnabled'] === 'true';
          _this.AckTimeout = data['AckTimeout'];
        }
      }
    });

    // $('#modal').hide();
  }

  MySensorsUpdateNode(nodeid: number) {
    if ($('#updelclr #nodeupdate').attr('class') === 'btnstyle3-dis') {
      return;
    }
    const name = this.nodename;

    this.mySensorsService.updateNode(this.devIdx, nodeid.toString(), name).subscribe(() => {
      this.RefreshMySensorsNodeTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Node!'), 2500, true);
    });
  }

  MySensorsDeleteNode(nodeid: number) {
    if ($('#updelclr #nodedelete').attr('class') === 'btnstyle3-dis') {
      return;
    }
    bootbox.confirm(this.translationService.t('Are you sure to remove this Node?'), (result) => {
      if (result === true) {
        this.mySensorsService.removeNode(this.devIdx, nodeid.toString()).subscribe(() => {
          this.RefreshMySensorsNodeTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem Deleting Node!'), 2500, true);
        });
      }
    });
  }

  MySensorsUpdateChild(nodeid: number, childid: any) {
    if ($('#updelclr #activedeviceupdate').attr('class') === 'btnstyle3-dis') {
      return;
    }
    const bUseAck = this.Ack;
    let AckTimeout = Number(this.AckTimeout);
    if (AckTimeout < 100) {
      AckTimeout = 100;
    } else if (AckTimeout > 10000) {
      AckTimeout = 10000;
    }
    this.mySensorsService.updateChild(this.devIdx, nodeid.toString(), childid, bUseAck, AckTimeout).subscribe(() => {
      this.MySensorsRefreshActiveDevicesTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Child!'), 2500, true);
    });
  }

  MySensorsDeleteChild(nodeid: number, childid: any) {
    if ($('#updelclr #activedevicedelete').attr('class') === 'btnstyle3-dis') {
      return;
    }
    bootbox.confirm(this.translationService.t('Are you sure to remove this Child?'), (result) => {
      if (result === true) {
        this.mySensorsService.removeChild(this.devIdx, nodeid.toString(), childid).subscribe(() => {
          this.MySensorsRefreshActiveDevicesTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem Deleting Child!'), 2500, true);
        });
      }
    });
  }

}
