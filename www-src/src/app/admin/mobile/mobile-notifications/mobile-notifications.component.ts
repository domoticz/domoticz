import { Component, OnInit, ViewChild, ElementRef, AfterViewInit, Inject } from '@angular/core';
import { DatatableHelper } from '../../../_shared/_utils/datatable-helper';
import { MobileService } from '../mobile.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { NotificationService } from '../../../_shared/_services/notification.service';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-mobile-notifications',
  templateUrl: './mobile-notifications.component.html',
  styleUrls: ['./mobile-notifications.component.css']
})
export class MobileNotificationsComponent implements OnInit, AfterViewInit {

  @ViewChild('mobiletable', { static: false }) mobiletableRef: ElementRef;

  selectedMobile: any;

  enabled: boolean;
  name: string;

  mobileupdateenabled = false;
  mobiledeleteenabled = false;

  constructor(
    private mobileService: MobileService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.ShowUsers();
  }

  private ShowUsers() {
    $(this.mobiletableRef.nativeElement).dataTable({
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
    });
    this.RefreshMobileTable();
  }

  private RefreshMobileTable() {
    this.mobileupdateenabled = false;
    this.mobiledeleteenabled = false;

    const oTable = $(this.mobiletableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.mobileService.getMobiles().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          let enabledstr = this.translationService.t('No');
          if (item.Enabled === 'true') {
            enabledstr = this.translationService.t('Yes');
          }
          let lUpdateItem = item.LastUpdate;
          lUpdateItem += '&nbsp;<img src="images/add.png" title="' + this.translationService.t('Test') +
            '" class=".js-send-test-message">';
          oTable.fnAddData({
            'DT_RowId': item.idx,
            'Enabled': item.Enabled,
            'Name': item.Name,
            'UUID': item.UUID,
            '0': item.idx,
            '1': enabledstr,
            '2': item.Name,
            '3': item.UUID,
            '4': item.DeviceType,
            '5': lUpdateItem
          });
        });
      }
    });

    const _this = this;

    oTable.on('click', '.js-send-test-message', function () {
      const mobile = oTable.api().row($(this).closest('tr')).data();

      _this.SendMobileTestMessage(mobile.DT_RowId);
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#mobiletable tbody').off();
    $('#mobiletable tbody').on('click', 'tr', function () {
      // $.devIdx = -1;

      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        _this.mobileupdateenabled = false;
        _this.mobiledeleteenabled = false;
        _this.selectedMobile = undefined;
      } else {
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        _this.mobileupdateenabled = true;
        _this.mobiledeleteenabled = true;
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          _this.selectedMobile = data;
          _this.enabled = _this.selectedMobile.Enabled === 'true';
          _this.name = _this.selectedMobile.Name;
        }
      }
    });
  }

  private SendMobileTestMessage(idx: string) {
    const subsystem = 'fcm';

    this.mobileService.testNotification(subsystem, 'midx_' + idx).subscribe(data => {
      if (data.status !== 'OK') {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Problem Sending Notification'), 3000, true);
        return;
      } else {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Notification sent!<br>Should arrive at your device soon...'), 3000);
      }
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('Problem Sending Notification'), 3000, true);
    });
  }

  UpdateMobile() {
    if (this.selectedMobile) {
      const idx = this.selectedMobile.DT_RowId;

      const csettings = this.GetMobileSettings();
      if (typeof csettings === 'undefined') {
        return;
      }

      this.mobileService.updateMobileDevice(idx, csettings.bEnabled, csettings.name).subscribe(() => {
        this.RefreshMobileTable();
      }, error => {
        this.notificationService.ShowNotify(this.translationService.t('Problem updating Mobile!'), 2500, true);
      });
    }
  }

  DeleteMobile() {
    if (this.selectedMobile) {
      const uuid = this.selectedMobile.UUID;

      bootbox.confirm(this.translationService.t('Are you sure to delete this Device?\n\nThis action can not be undone...'), (result) => {
        if (result === true) {
          this.mobileService.deleteMobileDevice(uuid).subscribe(() => {
            this.RefreshMobileTable();
          }, error => {
            this.notificationService.HideNotify();
            this.notificationService.ShowNotify(this.translationService.t('Problem deleting User!'), 2500, true);
          });
        }
      });
    }
  }

  private GetMobileSettings(): { bEnabled: boolean, name: string } | undefined {
    const csettings = {
      bEnabled: this.enabled,
      name: this.name
    };
    if (csettings.name === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
      return;
    }
    return csettings;
  }

}
