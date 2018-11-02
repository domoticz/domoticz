import {Component, Inject, Input, OnInit} from '@angular/core';
import {ConfigService} from 'src/app/_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {DialogService} from '../../_shared/_services/dialog.service';
import {SceneService} from '../../_shared/_services/scene.service';
import {mergeMap} from 'rxjs/operators';
import {Device} from "../../_shared/_models/device";
import {DomoticzDevicesService} from "../../_shared/_services/domoticz-devices.service";

@Component({
  selector: 'dz-dashboard-scene-widget',
  templateUrl: './dashboard-scene-widget.component.html',
  styleUrls: ['./dashboard-scene-widget.component.css']
})
export class DashboardSceneWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    private domoticzDevicesService: DomoticzDevicesService,
    private dialogService: DialogService,
    private sceneService: SceneService
  ) {
  }

  ngOnInit() {
  }

  get backgroundClass(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  get bigtext(): string {
    return this.deviceService.TranslateStatusShort(this.item.Status);
  }

  ShowCameraLiveStream() {
    this.domoticzDevicesService.ShowCameraLiveStream(this.item.Name, this.item.CameraIdx);
  }

  SwitchScene(switchcmd: string) {
    this.sceneService.SwitchScene(this.item.idx, switchcmd, this.item.Protected).pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe(updatedItem => {
      this.item = updatedItem;
    });
  }

}
