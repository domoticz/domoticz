import {AfterViewInit, Component, Inject, OnInit} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {ZwaveService} from '../zwave.service';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {HardwareService} from 'src/app/_shared/_services/hardware.service';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-hardware-z-wave-user-code-component',
  templateUrl: './hardware-z-wave-user-code-component.component.html',
  styleUrls: ['./hardware-z-wave-user-code-component.component.css']
})
export class HardwareZWaveUserCodeComponentComponent implements OnInit, AfterViewInit {

  idx: string;
  nodeidx: string;

  hardware: Hardware;

  constructor(
    private route: ActivatedRoute,
    private zwaveService: ZwaveService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private router: Router,
    private hardwareService: HardwareService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
    this.idx = this.route.snapshot.paramMap.get('idx');
    this.nodeidx = this.route.snapshot.paramMap.get('nodeidx');

    this.hardwareService.getHardware(this.idx).subscribe(hardware => {
      this.hardware = hardware;
    });
  }

  ngAfterViewInit() {
    $('#codestable').dataTable({
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
    this.RefreshOpenZWaveUserCodesTable();
  }

  RefreshOpenZWaveUserCodesTable() {
    const oTable = $('#codestable').dataTable();
    oTable.fnClearTable();
    this.zwaveService.getUserCodes(this.nodeidx).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          const removeButton = '<span class="label label-info lcursor removeusercode-click" removeusercodeindex="' +
            item.index + '">Remove</span>';
          oTable.fnAddData({
            'DT_RowId': item.index,
            'Code': item.index,
            'Value': item.code,
            '0': item.index,
            '1': item.code,
            '2': removeButton
          });
        });
      }
    });

    const _this = this;

    $('.removeusercode-click').off().on('click', function (e: Event) {
      e.preventDefault();
      _this.RemoveUserCode(this.removeusercodeindex);
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#codestable tbody').off();
    $('#codestable tbody').on('click', 'tr', function () {
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
      } else {
        // const oTable = $('#codestable').dataTable();
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          // var idx= data["DT_RowId"];
        }
      }
    });
  }

  RemoveUserCode(index: string) {
    bootbox.confirm(this.translationService.t('Are you sure to delete this User Code?'), (result) => {
      if (result === true) {
        this.zwaveService.removeUserCode(this.nodeidx, index).subscribe(() => {
          this.router.navigate(['/Hardware']);
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem deleting User Code!'), 2500, true);
        });
      }
    });
  }

}
