import { Component, OnInit, Input, Inject } from '@angular/core';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { DeviceService } from '../../_shared/_services/device.service';
import { LightSwitchesService } from '../../_shared/_services/light-switches.service';
import { mergeMap } from 'rxjs/operators';
import {Device} from "../../_shared/_models/device";

@Component({
  selector: '[dzDashboardMobileSecurityWiget]',
  templateUrl: './dashboard-mobile-security-widget.component.html',
  styleUrls: ['./dashboard-mobile-security-widget.component.css']
})
export class DashboardMobileSecurityWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    private lightSwitchesService: LightSwitchesService
  ) { }

  ngOnInit() {
  }

  get status(): string {
    return this.deviceService.TranslateStatus(this.item.Status);
  }

  get remoteTitle(): string {
    if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
      return 'Turn Alarm Off';
    } else {
      return 'Turn Alarm On';
    }
  }

  onRemoteClick() {
    if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
      this.lightSwitchesService.SwitchLight(this.item.idx, 'Off', this.item.Protected)
        .pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        )
        .subscribe(updatedItem => {
          this.item = updatedItem;
        });
    } else {
      this.lightSwitchesService.ArmSystem(this.item.idx, this.item.Protected)
        .pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        )
        .subscribe(updatedItem => {
          this.item = updatedItem;
        });
    }
  }

}
