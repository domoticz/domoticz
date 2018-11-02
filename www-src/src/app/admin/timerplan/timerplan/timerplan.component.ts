import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {SetupService} from '../../../_shared/_services/setup.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DatatableHelper} from '../../../_shared/_utils/datatable-helper';
import {DialogService} from '../../../_shared/_services/dialog.service';
import {AddEditPlanDialogComponent} from '../../../_shared/_dialogs/add-edit-plan-dialog/add-edit-plan-dialog.component';
import {RoomService} from '../../../_shared/_services/room.service';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-timerplan',
  templateUrl: './timerplan.component.html',
  styleUrls: ['./timerplan.component.css']
})
export class TimerplanComponent implements OnInit, AfterViewInit {

  @ViewChild('timerplantable', {static: false}) timerplantableRef: ElementRef;

  selectedTimer: TableRow = undefined;

  timerplaneditenabled = false;
  timerplancopyenabled = false;
  timerplandeleteenabled = false;

  constructor(private setupService: SetupService,
              private roomService: RoomService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private dialogService: DialogService,
              private notificationService: NotificationService,
              private configService: ConfigService) {
  }

  ngOnInit() {
  }

  ngAfterViewInit(): void {
    $(this.timerplantableRef.nativeElement).dataTable({
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single',
      },
      'bSort': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, 'All']],
      'iDisplayLength': 25,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });

    this.RefreshTimerPlanTable();
  }

  RefreshTimerPlanTable(): void {
    const _this = this;

    this.timerplaneditenabled = false;
    this.timerplancopyenabled = false;
    this.timerplandeleteenabled = false;

    const oTable = $(this.timerplantableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.setupService.getTimerPlans().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          let DisplayName = decodeURIComponent(item.Name);
          if (DisplayName === 'default') {
            DisplayName = this.translationService.t(DisplayName);
          }
          oTable.fnAddData(<TableRow>{
            'DT_RowId': item.idx,
            'Name': item.Name,
            '0': item.idx,
            '1': DisplayName
          });
        });
      }
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#timerplantable tbody').off();
    $('#timerplantable tbody').on('click', 'tr', function () {
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        _this.timerplaneditenabled = false;
        _this.timerplancopyenabled = false;
        _this.timerplandeleteenabled = false;
        _this.selectedTimer = undefined;
      } else {
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        _this.timerplaneditenabled = false;
        _this.timerplancopyenabled = false;
        _this.timerplandeleteenabled = false;

        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]) as TableRow;
          const idx = data.DT_RowId;
          // $.devIdx=idx;
          _this.selectedTimer = data;
          _this.timerplaneditenabled = true;
          _this.timerplancopyenabled = true;
          if (idx !== '0') {
            // not allowed to delete the default timer plan
            _this.timerplandeleteenabled = true;
          }
        }
      }
    });
  }

  EditTimerPlan() {
    const idx = this.selectedTimer.DT_RowId;

    let DisplayName = decodeURIComponent(this.selectedTimer.Name);
    if (DisplayName === 'default') {
      DisplayName = this.translationService.t(DisplayName);
    }

    const dialog = this.dialogService.addDialog(AddEditPlanDialogComponent, {
      title: 'Edit Plan',
      buttonTitle: 'Update',
      name: DisplayName,
      callbackFn: (name: string) => {
        this.roomService.updateTimerPlan(idx, name).subscribe(() => {
          this.RefreshTimerPlanTable();
        }, error => {
          this.notificationService.ShowNotify('Problem updating Plan settings!', 2500, true);
        });
      }
    }).instance;
    dialog.init();
    dialog.open();
  }

  CopyTimerPlan() {
    const idx = this.selectedTimer.DT_RowId;

    let DisplayName = decodeURIComponent(this.selectedTimer.Name);
    if (DisplayName === 'default') {
      DisplayName = this.translationService.t(DisplayName);
    }

    const dialog = this.dialogService.addDialog(AddEditPlanDialogComponent, {
      title: 'Duplicate Plan',
      buttonTitle: 'Duplicate',
      name: DisplayName,
      callbackFn: (name: string) => {
        this.roomService.duplicateTimerPlan(idx, name).subscribe(() => {
          this.RefreshTimerPlanTable();
        }, error => {
          this.notificationService.ShowNotify('Problem duplicating Plan!', 2500, true);
        });
      }
    }).instance;
    dialog.init();
    dialog.open();
  }

  DeleteTimerPlan() {
    const idx = this.selectedTimer.DT_RowId;

    bootbox.confirm(this.translationService.t('Are you sure you want to delete this Plan?'), (result: boolean) => {
      if (result === true) {
        this.roomService.deleteTimerPlan(idx).subscribe(() => {
          this.RefreshTimerPlanTable();
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify('Problem deleting Plan!', 2500, true);
        });
      }
    });
  }

  AddNewTimerPlan() {
    const dialog = this.dialogService.addDialog(AddEditPlanDialogComponent, {
      title: 'Add New Plan',
      buttonTitle: 'Add',
      callbackFn: (name: string) => {
        this.roomService.addTimerPlan(name).subscribe(() => {
          this.RefreshTimerPlanTable();
        }, error => {
          this.notificationService.ShowNotify('Problem adding Plan!', 2500, true);
        });
      }
    }).instance;
    dialog.init();
    dialog.open();
  }

}

interface TableRow {
  'DT_RowId': string;
  'Name': string;
  '0': string;
  '1': string;
}

