import {Component, Inject, Input, OnInit} from '@angular/core';
import {DeviceService} from '../../../_shared/_services/device.service';
import {PermissionService} from '../../../_shared/_services/permission.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {LivesocketService} from '../../../_shared/_services/livesocket.service';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-device-text-log',
  templateUrl: './device-text-log.component.html',
  styleUrls: ['./device-text-log.component.css']
})
export class DeviceTextLogComponent implements OnInit {

  @Input() deviceIdx: string;

  log: any; // FIXME type

  autoRefresh = true;

  constructor(
    private deviceService: DeviceService,
    private permissionService: PermissionService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private livesocketService: LivesocketService) {
  }

  ngOnInit() {
    this.refreshLog();

    this.livesocketService.device_update.subscribe(device => {
      if (this.autoRefresh && device.idx === this.deviceIdx) {
        this.refreshLog();
      }
    });
  }

  refreshLog() {
    this.deviceService.getTextLog(this.deviceIdx).subscribe(data => {
      for (let i = 0; i < data.result.length; i++) {
        const dataTemp = data.result[i]['Data'].replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
        data.result[i]['Data'] = dataTemp;
      }
      this.log = data.result;
    });
  }

  clearLog() {
    if (!this.permissionService.hasPermission('Admin')) {
      const message = this.translationService.t('You do not have permission to do that!');
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(message, 2500, true);
      return;
    }

    const _this = this;
    bootbox.confirm(this.translationService.t('Are you sure to delete the Log?\n\nThis action can not be undone!'), (result) => {
      if (result !== true) {
        return;
      }

      _this.deviceService.clearLightLog(_this.deviceIdx).subscribe(() => {
        _this.refreshLog();
      }, error => {
        _this.notificationService.HideNotify();
        _this.notificationService.ShowNotify(_this.translationService.t('Problem clearing the Log!'), 2500, true);
      });
    });
  }

}
