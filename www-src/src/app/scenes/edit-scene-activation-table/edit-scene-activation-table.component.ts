import {AfterViewInit, Component, ElementRef, EventEmitter, Input, OnInit, Output, ViewChild} from '@angular/core';
import {SceneService} from 'src/app/_shared/_services/scene.service';
import {SceneActivation} from 'src/app/_shared/_models/scene';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {ConfigService} from '../../_shared/_services/config.service';
import {Device} from '../../_shared/_models/device';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-edit-scene-activation-table',
  templateUrl: './edit-scene-activation-table.component.html',
  styleUrls: ['./edit-scene-activation-table.component.css']
})
export class EditSceneActivationTableComponent implements OnInit, AfterViewInit {

  @Input() item: Device;

  @Input() removeCodeClass: string;
  @Output() removeCodeClassChange: EventEmitter<string> = new EventEmitter<string>();

  @Output() select: EventEmitter<any> = new EventEmitter<any>();

  @ViewChild('table', {static: false}) tableRef: ElementRef;

  private datatable: any;

  constructor(
    private sceneService: SceneService,
    private configService: ConfigService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.datatable = $(this.tableRef.nativeElement).dataTable({
      sDom: '<"H"lfrC>t<"F"ip>',
      oTableTools: {
        sRowSelect: 'single',
      },
      aoColumnDefs: [
        {bSortable: false, aTargets: [1]}
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

    this.RefreshActivators();
  }

  public RefreshActivators() {
    this.removeCodeClassChange.emit('btnstyle3-dis');

    const oTable = this.datatable;
    oTable.fnClearTable();

    this.sceneService.getSceneActivations(this.item.idx).subscribe(response => {
      if (typeof response.result !== 'undefined') {
        response.result.forEach((item: SceneActivation) => {
          const addId = oTable.fnAddData({
            'DT_RowId': item.idx,
            'code': item.code,
            '0': item.idx,
            '1': item.name,
            '2': item.codestr
          });
        });
      }

      const _this = this;

      /* Add a click handler to the rows - this could be used as a callback */
      $('#scenecontent #scenedactivationtable tbody').off();
      $('#scenecontent #scenedactivationtable tbody').on('click', 'tr', function () {
        if ($(this).hasClass('row_selected')) {
          $(this).removeClass('row_selected');
          _this.removeCodeClassChange.emit('btnstyle3-dis');
        } else {
          oTable.$('tr.row_selected').removeClass('row_selected');
          $(this).addClass('row_selected');

          _this.removeCodeClassChange.emit('btnstyle3');
          const anSelected = DatatableHelper.fnGetSelected(oTable);
          if (anSelected.length !== 0) {
            const data = oTable.fnGetData(anSelected[0]);
            _this.select.emit(data);
          }
        }
      });
    });
  }

}
