import { Component, OnInit, Input, AfterViewInit, ViewChild, ElementRef, Output, EventEmitter, Inject } from '@angular/core';
import { SceneService } from '../../_shared/_services/scene.service';
import { SceneDevice } from '../../_shared/_models/scene';
import { Utils } from 'src/app/_shared/_utils/utils';
import { DatatableHelper } from 'src/app/_shared/_utils/datatable-helper';
import { PermissionService } from '../../_shared/_services/permission.service';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import {ConfigService} from "../../_shared/_services/config.service";
import {Device} from "../../_shared/_models/device";

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-edit-scene-devices-table',
  templateUrl: './edit-scene-devices-table.component.html',
  styleUrls: ['./edit-scene-devices-table.component.css']
})
export class EditSceneDevicesTableComponent implements OnInit, AfterViewInit {

  @Input() item: Device;

  @Input() deviceUpdateDeleteClass: string;
  @Output() deviceUpdateDeleteClassChange: EventEmitter<string> = new EventEmitter<string>();

  @Output() select: EventEmitter<any> = new EventEmitter<any>();

  @ViewChild('table', { static: false }) tableRef: ElementRef;

  datatable: any;

  constructor(
    private sceneService: SceneService,
    private permissionService: PermissionService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.datatable = $(this.tableRef.nativeElement).dataTable({
      sDom: '<"H"lfrC>t<"F"ip>',
      oTableTools: {
        sRowSelect: 'single',
      },
      aoColumnDefs: [
        { bSortable: false, aTargets: [1] }
      ],
      bSort: false,
      bProcessing: true,
      bStateSave: false,
      bJQueryUI: true,
      aLengthMenu: [[25, 50, 100, -1], [25, 50, 100, 'All']],
      iDisplayLength: 25,
      sPaginationType: 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });

    this.RefreshDeviceTable();
  }

  public RefreshDeviceTable() {
    this.deviceUpdateDeleteClassChange.emit('btnstyle3-dis');

    this.datatable.fnClearTable();

    this.sceneService.getSceneDevices(this.item.idx, this.item.Type === 'Scene').subscribe(response => {
      if (typeof response.result !== 'undefined') {
        const totalItems = response.result.length;

        response.result.forEach((item: SceneDevice, i: number) => {
          let command = '-';
          if (this.item.Type === 'Scene') {
            command = item.Command;
          }
          let updownImg = '';
          if (i !== totalItems - 1) {
            // Add Down Image
            if (updownImg !== '') {
              updownImg += '&nbsp;';
            }
            updownImg += '<img src="images/down.png" class="lcursor js-order-down" width="16" height="16"></img>';
          } else {
            updownImg += '<img src="images/empty16.png" width="16" height="16"></img>';
          }
          if (i !== 0) {
            // Add Up image
            if (updownImg !== '') {
              updownImg += '&nbsp;';
            }
            updownImg += '<img src="images/up.png" class="lcursor js-order-up" width="16" height="16"></img>';
          }
          let levelstr = item.Level + ' %';

          if (Utils.isLED(item.SubType)) {
            let color: any = {};
            try {
              color = JSON.parse(item.Color);
            } catch (e) {
              // forget about it :)
            }
            // TODO: Refactor to some nice helper function, ensuring range of 0..ff etc
            // TODO: Calculate color if color mode is white/temperature.
            let rgbhex = '808080';
            if (color.m === 1 || color.m === 2) { // White or color temperature
              let whex = Math.round(255 * item.Level / 100).toString(16);
              if (whex.length === 1) {
                whex = '0' + whex;
              }
              rgbhex = whex + whex + whex;
            }
            if (color.m === 3 || color.m === 4) { // RGB or custom
              let rhex = Math.round(color.r).toString(16);
              if (rhex.length === 1) {
                rhex = '0' + rhex;
              }
              let ghex = Math.round(color.g).toString(16);
              if (ghex.length === 1) {
                ghex = '0' + ghex;
              }
              let bhex = Math.round(color.b).toString(16);
              if (bhex.length === 1) {
                bhex = '0' + bhex;
              }
              rgbhex = rhex + ghex + bhex;
            }
            levelstr += '<div id="picker4" class="ex-color-box" style="background-color: #' + rgbhex + ';"></div>';
          }

          const addId = this.datatable.fnAddData({
            'DT_RowId': item.ID,
            'Command': item.Command,
            'RealIdx': item.DevRealIdx,
            'Level': item.Level,
            'Color': item.Color,
            'OnDelay': item.OnDelay,
            'OffDelay': item.OffDelay,
            'Order': item.Order,
            'IsScene': item.Order,
            '0': item.Name,
            '1': command,
            '2': levelstr,
            '3': item.OnDelay,
            '4': item.OffDelay,
            '5': updownImg
          });

        });
      }

      const _this = this;

      this.datatable.off();
      this.datatable.on('click', '.js-order-up', function () {
        const row = _this.datatable.api().row($(this).closest('tr')).data();
        _this.ChangeDeviceOrder(0, row['DT_RowId']);
      });
      this.datatable.on('click', '.js-order-down', function () {
        const row = _this.datatable.api().row($(this).closest('tr')).data();
        _this.ChangeDeviceOrder(1, row['DT_RowId']);
      });

      /* Add a click handler to the rows - this could be used as a callback */
      $('#scenecontent #scenedevicestable tbody').off();
      $('#scenecontent #scenedevicestable tbody').on('click', 'tr', function () {
        if ($(this).hasClass('row_selected')) {
          $(this).removeClass('row_selected');
          _this.deviceUpdateDeleteClassChange.emit('btnstyle3-dis');
        } else {
          const oTable = _this.datatable;
          oTable.$('tr.row_selected').removeClass('row_selected');
          $(this).addClass('row_selected');

          _this.deviceUpdateDeleteClassChange.emit('btnstyle3');
          // $('#scenecontent #delclr #updatedelete').show();

          const anSelected = DatatableHelper.fnGetSelected(oTable);
          if (anSelected.length !== 0) {
            const data = oTable.fnGetData(anSelected[0]);
            _this.select.emit(data);
          }
        }
      });

    });
  }

  private ChangeDeviceOrder(order: number, devid: string) {
    if (!this.permissionService.hasPermission('Admin')) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return;
    }
    this.sceneService.changeSceneDeviceOrder(devid, order.toString()).subscribe(() => {
      this.RefreshDeviceTable();
    });
  }

}
