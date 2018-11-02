import { Component, OnInit, ViewChild, ElementRef, Inject, AfterViewInit } from '@angular/core';
import { UsersService } from 'src/app/_shared/_services/users.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { DatatableHelper } from '../../../_shared/_utils/datatable-helper';
import { NotificationService } from '../../../_shared/_services/notification.service';
import { Md5 } from 'ts-md5';
import { Router } from '@angular/router';
import {ConfigService} from "../../../_shared/_services/config.service";

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

const defaultUserEdit: UserEdit = {
  idx: '',
  Enabled: true,
  Username: '',
  Password: '',
  Rights: 0,
  Sharing: false,
  EnableTabLights: false,
  EnableTabScenes: false,
  EnableTabTemp: false,
  EnableTabWeather: false,
  EnableTabUtility: false,
  EnableTabCustom: false,
  EnableTabFloorplans: false
};

@Component({
  selector: 'dz-users',
  templateUrl: './users.component.html',
  styleUrls: ['./users.component.css']
})
export class UsersComponent implements OnInit, AfterViewInit {

  @ViewChild('usertable', { static: false }) userTableRef: ElementRef;
  userTable: any;

  selectedUser: UserEdit = { ...defaultUserEdit };

  constructor(
    private usersService: UsersService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private router: Router,
    private configService: ConfigService
  ) { }

  ngOnInit() { }

  ngAfterViewInit() {
    this.ShowUsers();
  }

  private ShowUsers() {
    // $.devIdx = -1;

    const dataTableOptions = {
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single',
      },
      'aoColumnDefs': [
        { 'bSortable': false, 'aTargets': [0] }
      ],
      'aaSorting': [[1, 'asc']],
      'bSortClasses': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, 'All']],
      'iDisplayLength': 25,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    };

    this.userTable = $(this.userTableRef.nativeElement).dataTable(dataTableOptions);

    this.RefreshUserTable();
  }

  RefreshUserTable() {
    // $.devIdx = -1;

    // FIXME handle with Angular
    $('#updelclr #userupdate').attr('class', 'btnstyle3-dis');
    $('#updelclr #userdelete').attr('class', 'btnstyle3-dis');

    this.userTable.fnClearTable();

    this.usersService.getUsers().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          let enabledstr = this.translationService.t('No');
          if (item.Enabled === 'true') {
            enabledstr = this.translationService.t('Yes');
          }

          let rightstr = 'rights';
          if (item.Rights === 0) {
            rightstr = this.translationService.t('Viewer');
          } else if (item.Rights === 1) {
            rightstr = this.translationService.t('User');
          } else {
            rightstr = this.translationService.t('Admin');
          }

          let sharedstr = this.translationService.t('No');
          if (item.RemoteSharing !== 0) {
            sharedstr = this.translationService.t('Yes');
          }

          const devicesstr = '<span class="label label-info js-set-devices">' + this.translationService.t('Set Devices') + '</span>';
          this.userTable.fnAddData({
            'DT_RowId': item.idx,
            'Enabled': item.Enabled,
            'Username': item.Username,
            'Password': item.Password,
            'Rights': item.Rights,
            'Sharing': item.RemoteSharing,
            'TabsEnabled': item.TabsEnabled,
            '0': enabledstr,
            '1': item.Username,
            '2': rightstr,
            '3': sharedstr,
            '4': devicesstr
          });
        });
      }
    });

    const _this = this;

    this.userTable.off();
    this.userTable.on('click', '.js-set-devices', function () {
      const row = _this.userTable.api().row($(this).closest('tr')).data();
      _this.router.navigate(['/UserDevices', row['DT_RowId'], row['Username']]);
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#usertable tbody').off();
    $('#usertable tbody').on('click', 'tr', function () {
      // $.devIdx = -1;

      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        $('#updelclr #userupdate').attr('class', 'btnstyle3-dis');
        $('#updelclr #userdelete').attr('class', 'btnstyle3-dis');
        _this.selectedUser = { ...defaultUserEdit };
      } else {
        const oTable = _this.userTable;
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        $('#updelclr #userupdate').attr('class', 'btnstyle3');
        $('#updelclr #userdelete').attr('class', 'btnstyle3');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]) as UserRow;
          const idx = data['DT_RowId'];
          // $.devIdx = idx;
          _this.selectedUser = {
            idx: idx,
            Enabled: data.Enabled === 'true',
            Username: data.Username,
            Password: data.Password,
            Rights: data.Rights,
            Sharing: data.Sharing === 1,
            // tslint:disable:no-bitwise
            EnableTabLights: (data.TabsEnabled & 1) !== 0,
            EnableTabScenes: (data.TabsEnabled & 2) !== 0,
            EnableTabTemp: (data.TabsEnabled & 4) !== 0,
            EnableTabWeather: (data.TabsEnabled & 8) !== 0,
            EnableTabUtility: (data.TabsEnabled & 16) !== 0,
            EnableTabCustom: (data.TabsEnabled & 32) !== 0,
            EnableTabFloorplans: (data.TabsEnabled & 64) !== 0,
          };
        }
      }
    });
  }

  UpdateUser() {
    if (this.selectedUser) {
      const csettings = this.GetUserSettings();
      if (typeof csettings === 'undefined') {
        return;
      }

      this.usersService.updateUser(this.selectedUser.idx, csettings.bEnabled, csettings.username, csettings.password, csettings.rights,
        csettings.bEnableSharing, csettings.TabsEnabled).subscribe(() => {
          this.RefreshUserTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating User!'), 2500, true);
        });
    }
  }

  GetUserSettings() {
    const csettings: any = {};

    csettings.bEnabled = this.selectedUser.Enabled;
    csettings.username = this.selectedUser.Username;
    if (csettings.username === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Username!'), 2500, true);
      return;
    }
    csettings.password = this.selectedUser.Password;
    if (csettings.password === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Password!'), 2500, true);
      return;
    }
    if (csettings.password.length !== 32) {
      csettings.password = Md5.hashStr(csettings.password);
    }
    csettings.rights = this.selectedUser.Rights;
    csettings.bEnableSharing = this.selectedUser.Sharing;

    csettings.TabsEnabled = 0;
    if (this.selectedUser.EnableTabLights) {
      csettings.TabsEnabled |= (1 << 0);
    }
    if (this.selectedUser.EnableTabScenes) {
      csettings.TabsEnabled |= (1 << 1);
    }
    if (this.selectedUser.EnableTabTemp) {
      csettings.TabsEnabled |= (1 << 2);
    }
    if (this.selectedUser.EnableTabWeather) {
      csettings.TabsEnabled |= (1 << 3);
    }
    if (this.selectedUser.EnableTabUtility) {
      csettings.TabsEnabled |= (1 << 4);
    }
    if (this.selectedUser.EnableTabCustom) {
      csettings.TabsEnabled |= (1 << 5);
    }
    if (this.selectedUser.EnableTabFloorplans) {
      csettings.TabsEnabled |= (1 << 6);
    }

    return csettings;
  }

  DeleteUser() {
    if (this.selectedUser) {
      bootbox.confirm(this.translationService.t('Are you sure you want to delete this User?'), (result) => {
        if (result === true) {
          this.usersService.deleteUser(this.selectedUser.idx).subscribe(() => {
            this.RefreshUserTable();
          }, error => {
            this.notificationService.HideNotify();
            this.notificationService.ShowNotify(this.translationService.t('Problem deleting User!'), 2500, true);
          });
        }
      });
    }
  }

  AddUser() {
    const csettings = this.GetUserSettings();
    if (typeof csettings === 'undefined') {
      return;
    }

    this.usersService.addUser(csettings.bEnabled, csettings.username, csettings.password, csettings.rights, csettings.bEnableSharing,
      csettings.TabsEnabled).subscribe((data) => {
        if (data.status !== 'OK') {
          this.notificationService.ShowNotify(data.message, 2500, true);
          return;
        }
        this.RefreshUserTable();
      }, error => {
        this.notificationService.ShowNotify(this.translationService.t('Problem adding User!'), 2500, true);
      });
  }

}

interface UserRow {
  'DT_RowId': string;
  'Enabled': string;
  'Username': string;
  'Password': string;
  'Rights': number;
  'Sharing': number;
  'TabsEnabled': number;
  '0': string;
  '1': string;
  '2': string;
  '3': string;
  '4': string;
}

interface UserEdit {
  idx: string;
  Enabled: boolean;
  Username: string;
  Password: string;
  Rights: number;
  Sharing: boolean;
  EnableTabLights: boolean;
  EnableTabScenes: boolean;
  EnableTabTemp: boolean;
  EnableTabWeather: boolean;
  EnableTabUtility: boolean;
  EnableTabCustom: boolean;
  EnableTabFloorplans: boolean;
}

