import { Component, OnInit, Input, Inject } from '@angular/core';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { DeviceService } from 'src/app/_shared/_services/device.service';
import { Utils } from 'src/app/_shared/_utils/utils';
import { LightSwitchesService } from '../../_shared/_services/light-switches.service';
import { mergeMap } from 'rxjs/operators';
import {Device} from "../../_shared/_models/device";

@Component({
  selector: 'dz-dashboard-security-widget',
  templateUrl: './dashboard-security-widget.component.html',
  styleUrls: ['./dashboard-security-widget.component.css']
})
export class DashboardSecurityWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    private lightSwitchesService: LightSwitchesService
  ) { }

  ngOnInit() {
  }

  get backgroundClass(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  get bigtext(): string {
    return this.deviceService.TranslateStatusShort(this.item.Status);
  }

  get imageSrc(): string {
    if (this.item.SubType.indexOf('remote') > 0) {
      if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
        return 'remote48.png';
      } else {
        return 'remote48.png';
      }
    } else if (this.item.SwitchType === 'Smoke Detector') {
      if (
        (this.item.Status === 'Panic') ||
        (this.item.Status === 'On')
      ) {
        return 'smoke48on.png';
      } else {
        return 'smoke48off.png';
      }
    } else if (this.item.SubType === 'X10 security') {
      if (this.item.Status.indexOf('Normal') >= 0) {
        return 'security48.png';
      } else {
        return 'Alarm48_On.png';
      }
    } else if (this.item.SubType === 'X10 security motion') {
      if ((this.item.Status === 'No Motion')) {
        return 'security48.png';
      } else {
        return 'Alarm48_On.png';
      }
    } else if ((this.item.Status.indexOf('Alarm') >= 0) || (this.item.Status.indexOf('Tamper') >= 0)) {
      return 'Alarm48_On.png';
    } else if (this.item.SubType.indexOf('Meiantech') >= 0) {
      if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
        return 'security48.png';
      } else {
        return 'security48.png';
      }
    } else {
      if (this.item.SubType.indexOf('KeeLoq') >= 0) {
        return 'pushon48.png';
      } else {
        return 'security48.png';
      }
    }
  }

  get imageTitle(): string {
    if (this.item.SubType.indexOf('remote') > 0) {
      if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
        return 'Turn Alarm Off';
      } else {
        return 'Turn Alarm On';
      }
    } else if (this.item.SwitchType === 'Smoke Detector') {
      if (
        (this.item.Status === 'Panic') ||
        (this.item.Status === 'On')
      ) {
        return '';
      } else {
        return '';
      }
    } else if (this.item.SubType === 'X10 security') {
      if (this.item.Status.indexOf('Normal') >= 0) {
        return 'Turn Alarm On';
      } else {
        return 'Turn Alarm Off';
      }
    } else if (this.item.SubType === 'X10 security motion') {
      if ((this.item.Status === 'No Motion')) {
        return 'Turn Alarm On';
      } else {
        return 'Turn Alarm Off';
      }
    } else if ((this.item.Status.indexOf('Alarm') >= 0) || (this.item.Status.indexOf('Tamper') >= 0)) {
      return '';
    } else if (this.item.SubType.indexOf('Meiantech') >= 0) {
      if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
        return 'Turn Alarm Off';
      } else {
        return 'Turn Alarm On';
      }
    } else {
      if (this.item.SubType.indexOf('KeeLoq') >= 0) {
        return 'Turn On';
      } else {
        return '';
      }
    }
  }

  onImageClick() {
    if (this.item.SubType.indexOf('remote') > 0) {
      if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
        this.lightSwitchesService.SwitchLight(this.item.idx, 'Off', this.item.Protected).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe(updatedItem => {
          this.item = updatedItem;
        });
      } else {
        this.lightSwitchesService.ArmSystem(this.item.idx, this.item.Protected).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe(updatedItem => {
          this.item = updatedItem;
        });
      }
    } else if (this.item.SwitchType === 'Smoke Detector') {
      if (
        (this.item.Status === 'Panic') ||
        (this.item.Status === 'On')
      ) {
        // Nothing
      } else {
        // Nothing
      }
    } else if (this.item.SubType === 'X10 security') {
      if (this.item.Status.indexOf('Normal') >= 0) {
        this.lightSwitchesService.SwitchLight(this.item.idx, (this.item.Status === 'Normal Delayed') ? 'Alarm Delayed' : 'Alarm',
          this.item.Protected).pipe(
            mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
          ).subscribe(updatedItem => {
            this.item = updatedItem;
          });
      } else {
        this.lightSwitchesService.SwitchLight(this.item.idx, (this.item.Status === 'Alarm Delayed') ? 'Normal Delayed' : 'Normal',
          this.item.Protected).pipe(
            mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
          ).subscribe(updatedItem => {
            this.item = updatedItem;
          });
      }
    } else if (this.item.SubType === 'X10 security motion') {
      if ((this.item.Status === 'No Motion')) {
        this.lightSwitchesService.SwitchLight(this.item.idx, 'Motion', this.item.Protected).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe(updatedItem => {
          this.item = updatedItem;
        });
      } else {
        this.lightSwitchesService.SwitchLight(this.item.idx, 'No Motion', this.item.Protected).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe(updatedItem => {
          this.item = updatedItem;
        });
      }
    } else if ((this.item.Status.indexOf('Alarm') >= 0) || (this.item.Status.indexOf('Tamper') >= 0)) {
      // Nothing
    } else if (this.item.SubType.indexOf('Meiantech') >= 0) {
      if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
        this.lightSwitchesService.SwitchLight(this.item.idx, 'Off', this.item.Protected).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe(updatedItem => {
          this.item = updatedItem;
        });
      } else {
        this.lightSwitchesService.ArmSystemMeiantech(this.item.idx, this.item.Protected).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe(updatedItem => {
          this.item = updatedItem;
        });
      }
    } else {
      if (this.item.SubType.indexOf('KeeLoq') >= 0) {
        this.lightSwitchesService.SwitchLight(this.item.idx, 'On', this.item.Protected).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe(updatedItem => {
          this.item = updatedItem;
        });
      } else {
        // Nothing
      }
    }
  }

}
