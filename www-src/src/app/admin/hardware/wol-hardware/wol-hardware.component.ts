import {AfterViewInit, Component, Inject, Input, OnInit} from '@angular/core';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {WolService} from '../wol.service';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {WolNode} from '../wol';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-wol-hardware',
  templateUrl: './wol-hardware.component.html',
  styleUrls: ['./wol-hardware.component.css']
})
export class WolHardwareComponent implements OnInit, AfterViewInit {

  @Input() hardware: Hardware;

  nodename = '';
  nodemac = '';
  selectedRow: WolNode;

  constructor(
    private wolService: WolService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
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
    this.RefreshWOLNodeTable();
  }

  RefreshWOLNodeTable() {
    $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
    $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
    this.nodename = '';
    this.nodemac = '';

    const oTable = $('#nodestable').dataTable();
    oTable.fnClearTable();

    this.wolService.getNodes(this.hardware.idx.toString()).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item) => {
          oTable.fnAddData({
            'DT_RowId': item.idx,
            'Name': item.Name,
            'Mac': item.Mac,
            '0': item.idx,
            '1': item.Name,
            '2': item.Mac
          });
        });
      }
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#nodestable tbody').off();
    $('#nodestable tbody').on('click', 'tr', function () {
      $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
        _this.nodename = '';
        _this.nodemac = '';
      } else {
        // var oTable = $('#nodestable').dataTable();
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        $('#updelclr #nodeupdate').attr('class', 'btnstyle3');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const idx = data['DT_RowId'];
          _this.selectedRow = {
            idx: idx,
            Name: data['1'],
            Mac: data['2']
          };
          $('#updelclr #nodedelete').attr('class', 'btnstyle3');
          _this.nodename = _this.selectedRow.Name;
          _this.nodemac = _this.selectedRow.Mac;
        }
      }
    });
  }

  WOLClearNodes() {
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Nodes?\n\nThis action can not be undone!'), (result) => {
      if (result === true) {
        this.wolService.clearNodes(this.hardware.idx.toString()).subscribe(() => {
          this.RefreshWOLNodeTable();
        });
      }
    });
  }

  AddWOLNode() {
    const name = this.nodename;
    if (name === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
      return;
    }
    const mac = this.nodemac;
    if (mac === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a MAC Address!'), 2500, true);
      return;
    }

    this.wolService.addNode(this.hardware.idx.toString(), name, mac).subscribe(() => {
      this.RefreshWOLNodeTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Adding Node!'), 2500, true);
    });
  }

  WOLUpdateNode() {
    if (this.selectedRow) {
      if ($('#updelclr #nodedelete').attr('class') === 'btnstyle3-dis') {
        return;
      }

      const name = this.nodename;
      if (name === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
        return;
      }
      const mac = this.nodemac;
      if (mac === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a MAC Address!'), 2500, true);
        return;
      }

      this.wolService.updateNode(this.hardware.idx.toString(), this.selectedRow.idx, name, mac).subscribe(() => {
        this.RefreshWOLNodeTable();
      }, error => {
        this.notificationService.ShowNotify(this.translationService.t('Problem Updating Node!'), 2500, true);
      });
    }
  }

  WOLDeleteNode() {
    if (this.selectedRow) {
      if ($('#updelclr #nodedelete').attr('class') === 'btnstyle3-dis') {
        return;
      }
      bootbox.confirm(this.translationService.t('Are you sure to remove this Node?'), (result) => {
        if (result === true) {
          this.wolService.removeNode(this.hardware.idx.toString(), this.selectedRow.idx).subscribe(() => {
            this.RefreshWOLNodeTable();
          }, error => {
            this.notificationService.ShowNotify(this.translationService.t('Problem Deleting Node!'), 2500, true);
          });
        }
      });
    }
  }

}
