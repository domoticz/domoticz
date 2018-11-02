import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {PermissionService} from '../../../_shared/_services/permission.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {LivesocketService} from '../../../_shared/_services/livesocket.service';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-device-light-log',
  templateUrl: './device-light-log.component.html',
  styleUrls: ['./device-light-log.component.css']
})
export class DeviceLightLogComponent implements OnInit, AfterViewInit {

  @Input() deviceIdx: string;

  @ViewChild('lightgraph', {static: false}) lightgraphElt: ElementRef;

  log: any; // FIXME type
  isSelector: boolean;
  isDimmer: boolean;

  autoRefresh = true;

  constructor(
    private deviceService: DeviceService,
    private permissionService: PermissionService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.refreshLog();

    this.livesocketService.device_update.subscribe((device) => {
      if (this.autoRefresh && device.idx === this.deviceIdx) {
        this.refreshLog();
      }
    });
  }

  private refreshLog() {
    this.deviceService.getLightLog(this.deviceIdx).subscribe(data => {
      if (typeof data.result === 'undefined') {
        return;
      }

      this.log = (data.result || []).sort((a, b) => {
        return a.Date < b.Date ? -1 : 1;
      });

      this.isSelector = data.HaveSelector;
      this.isDimmer = data.HaveDimmer;
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
