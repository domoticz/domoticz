import {Component, Inject, Input, OnInit} from '@angular/core';
import {Utils} from 'src/app/_shared/_utils/utils';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {DialogService} from '../../_shared/_services/dialog.service';
import {PermissionService} from '../../_shared/_services/permission.service';
import {NotificationService} from '../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {SceneService} from '../../_shared/_services/scene.service';
import {mergeMap} from 'rxjs/operators';
import {ConfigService} from '../../_shared/_services/config.service';
import {timer} from 'rxjs';
import {Device} from '../../_shared/_models/device';
import {DomoticzDevicesService} from '../../_shared/_services/domoticz-devices.service';

@Component({
  selector: 'dz-scene-widget',
  templateUrl: './scene-widget.component.html',
  styleUrls: ['./scene-widget.component.css']
})
export class SceneWidgetComponent implements OnInit {

  @Input() item: Device;

  highlight = false;

  constructor(
    private deviceService: DeviceService,
    private domoticzDevicesService: DomoticzDevicesService,
    private dialogService: DialogService,
    private permissionService: PermissionService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private sceneService: SceneService,
    private configService: ConfigService
  ) {
  }

  ngOnInit() {
    // Trigger highlight effect when creating the component
    // (the component get recreated when RefreshWeather is called on dashboard page)
    this.highlightItem();
  }

  private highlightItem() {
    if (this.configService.config.ShowUpdatedEffect === true) {
      this.highlight = true;
      timer(1000).subscribe(_ => this.highlight = false);
    }
  }

  nbackstyle(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  get bigtext(): string {
    return this.deviceService.TranslateStatusShort(this.item.Status);
  }

  ShowCameraLiveStream() {
    this.domoticzDevicesService.ShowCameraLiveStream(this.item.Name, this.item.CameraIdx);
  }

  SwitchScene(cmd: string) {
    this.sceneService.SwitchScene(this.item.idx, cmd, this.item.Protected)
      .pipe(
        mergeMap(() => this.sceneService.getScene(this.item.idx))
      )
      .subscribe(updatedItem => {
        this.item = updatedItem;
      });
  }

  MakeFavorite(isfavorite: number): void {
    if (!this.permissionService.hasPermission('Admin')) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return;
    }

    this.sceneService.makeFavorite(Number(this.item.idx), isfavorite).subscribe(_ => {
      this.item.Favorite = isfavorite;
    });
  }

}
