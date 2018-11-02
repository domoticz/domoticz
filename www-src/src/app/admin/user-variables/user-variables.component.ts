import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {UserVariablesService} from '../../_shared/_services/user-variables.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DatatableHelper} from '../../_shared/_utils/datatable-helper';
import {NotificationService} from '../../_shared/_services/notification.service';
import {ApiResponse} from '../../_shared/_models/api';
import {ConfigService} from '../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-user-variables',
  templateUrl: './user-variables.component.html',
  styleUrls: ['./user-variables.component.css']
})
export class UserVariablesComponent implements OnInit, AfterViewInit {

  varNames: Array<string> = [];

  @ViewChild('uservariablestable', {static: false}) uservariablestableRef: ElementRef;

  editMode = false;
  userVariable: { idx: string; name: string; type: string; value: string; } = {
    idx: '',
    name: '',
    type: '0',
    value: ''
  };

  constructor(
    private userVariablesService: UserVariablesService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private configService: ConfigService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit(): void {
    this.ShowUserVariables();
  }

  private ShowUserVariables(): void {
    $(this.uservariablestableRef.nativeElement).dataTable({
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

    this.RefreshUserVariablesTable();
  }

  RefreshUserVariablesTable(): void {
    this.editMode = false;

    this.varNames = [];
    const oTable = $(this.uservariablestableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.userVariablesService.getUserVariables().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          this.varNames.push(item.Name);
          let typeWording;
          switch (item.Type) {
            case '0':
              typeWording = this.translationService.t('Integer');
              break;
            case '1':
              typeWording = this.translationService.t('Float');
              break;
            case '2':
              typeWording = this.translationService.t('String');
              break;
            case '3':
              typeWording = this.translationService.t('Date');
              break;
            case '4':
              typeWording = this.translationService.t('Time');
              break;
            case '5':
              typeWording = this.translationService.t('DateTime');
              break;
            default:
              typeWording = 'undefined';
          }
          oTable.fnAddData({
            'DT_RowId': item.idx,
            'DT_ItemType': item.Type,
            '0': item.idx,
            '1': item.Name,
            '2': typeWording,
            '3': item.Value,
            '4': item.LastUpdate
          });
        });
      }
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#uservariablestable tbody').off();
    $('#uservariablestable tbody').on('click', 'tr', function () {
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        _this.editMode = false;
      } else {
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const idx = data['DT_RowId'];
          _this.editMode = true;
          _this.userVariable = {
            idx: data['DT_RowID'],
            name: data['1'],
            type: data['DT_ItemType'],
            value: data['3']
          };
        }
      }
    });
  }

  AddVariable() {
    if (this.varNames.includes(this.userVariable.name)) {
      this.notificationService.ShowNotify(this.translationService.t('Variable name already exists!'), 2500, true);
    } else {
      this.userVariablesService.addUserVariable(this.userVariable.name, this.userVariable.type, this.userVariable.value).subscribe((data) => {
        this.successAddOrUpdate(data);
      }, error => {
        this.notificationService.ShowNotify(this.translationService.t('Problem saving user variable!'), 2500, true);
      });
    }
  }

  UpdateVariable(idx: string) {
    this.userVariablesService.updateUserVariable(idx, this.userVariable.name, this.userVariable.type, this.userVariable.value).subscribe((data) => {
      this.successAddOrUpdate(data);
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving user variable!'), 2500, true);
    });
  }

  private successAddOrUpdate(data: ApiResponse) {
    if (typeof data !== 'undefined') {
      if (data.status === 'OK') {
        bootbox.alert(this.translationService.t('User variable saved'));
        this.RefreshUserVariablesTable();
        this.userVariable.name = '';
        this.userVariable.value = '';
        this.userVariable.type = '0';
      } else {
        this.notificationService.ShowNotify(this.translationService.t(data.message), 2500, true);
      }
    }
  }

  DeleteVariable(idx: string) {
    bootbox.confirm(this.translationService.t('Are you sure you want to remove this variable?'), (result) => {
      if (result === true) {
        this.userVariablesService.deleteUserVariable(idx).subscribe(() => {
          this.RefreshUserVariablesTable();
        });
      }
    });
  }

}
