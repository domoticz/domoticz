import {AfterViewInit, Component, EventEmitter, Inject, Input, OnInit, Output, ViewChild} from '@angular/core';
import {ConfigService} from '../../_shared/_services/config.service';
import {timer} from 'rxjs';
import {Utils} from 'src/app/_shared/_utils/utils';
import {DeviceService} from '../../_shared/_services/device.service';
import {PermissionService} from '../../_shared/_services/permission.service';
import {NotificationService} from '../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DimmerSliderComponent} from '../../_shared/_components/dimmer-slider/dimmer-slider.component';
import {LightSwitchesService} from '../../_shared/_services/light-switches.service';
import {mergeMap} from 'rxjs/operators';
import {SelectorlevelsComponent} from '../../_shared/_components/selectorlevels/selectorlevels.component';
import {RfyPopupComponent} from '../rfy-popup/rfy-popup.component';
import {DialogService} from '../../_shared/_services/dialog.service';
import {RgbwPopupComponent} from '../../_shared/_components/rgbw-popup/rgbw-popup.component';
import {LucciPopupComponent} from '../../_shared/_components/lucci-popup/lucci-popup.component';
import {IthoPopupComponent} from '../../_shared/_components/itho-popup/itho-popup.component';
import {Therm3PopupComponent} from '../../_shared/_components/therm3-popup/therm3-popup.component';
import {Device} from '../../_shared/_models/device';
import {DomoticzDevicesService} from '../../_shared/_services/domoticz-devices.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-light-switch-widget',
  templateUrl: './light-switch-widget.component.html',
  styleUrls: ['./light-switch-widget.component.css']
})
export class LightSwitchWidgetComponent implements OnInit, AfterViewInit {

  @Input() item: Device;

  @Output() private replaceDevice: EventEmitter<void> = new EventEmitter<void>();
  @Output() private removeDevice: EventEmitter<void> = new EventEmitter<void>();

  highlight = false;

  @ViewChild(DimmerSliderComponent, {static: false}) dimmerSlider?: DimmerSliderComponent;
  @ViewChild(SelectorlevelsComponent, {static: false}) selectorlevels?: SelectorlevelsComponent;

  @ViewChild(RfyPopupComponent, {static: false}) rfyPopup?: RfyPopupComponent;
  @ViewChild(RgbwPopupComponent, {static: false}) rgbwPopup?: RgbwPopupComponent;
  @ViewChild(LucciPopupComponent, {static: false}) lucciPopup?: LucciPopupComponent;
  @ViewChild(IthoPopupComponent, {static: false}) ithoPopup?: IthoPopupComponent;
  @ViewChild(Therm3PopupComponent, {static: false}) therm3Popup?: Therm3PopupComponent;

  evohomeSelected = false;

  evohomePopupObjects: Array<{ name: string, data: string }> = [
    {name: 'Normal', data: 'Auto'},
    {name: 'Economy', data: 'AutoWithEco'},
    {name: 'Away', data: 'Away'},
    {name: 'Day Off', data: 'DayOff'},
    {name: 'Custom', data: 'Custom'},
    {name: 'Heating Off', data: 'HeatingOff'}
  ];

  constructor(
    private configService: ConfigService,
    private deviceService: DeviceService,
    private domoticzDevicesService: DomoticzDevicesService,
    private permissionService: PermissionService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private lightSwitchesService: LightSwitchesService,
    private dialogService: DialogService
  ) {
  }

  ngOnInit() {
    // Trigger highlight effect when creating the component
    // (the component get recreated when RefreshUtilities is called on dashboard page)
    this.highlightItem();
  }

  ngAfterViewInit() {
    if (this.dimmerSlider) {
      this.dimmerSlider.ResizeDimSliders();
    }
    if (this.selectorlevels) {
      this.selectorlevels.Resize();
    }
  }

  private highlightItem() {
    if (this.configService.config.ShowUpdatedEffect === true) {
      this.highlight = true;
      timer(1000).subscribe(_ => this.highlight = false);
    }
  }

  get tableId(): string {
    if ((this.item.SwitchType === 'Blinds') ||
      (this.item.SwitchType === 'Blinds Inverted') ||
      (this.item.SwitchType === 'Blinds Percentage') ||
      (this.item.SwitchType === 'Blinds Percentage Inverted') ||
      (this.item.SwitchType.indexOf('Venetian Blinds') === 0) ||
      (this.item.SwitchType.indexOf('Media Player') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        return 'itemtabletrippleicon';
      } else {
        return 'itemtabledoubleicon';
      }
    } else {
      return 'itemtablenostatus';
    }
  }

  nbackstyle(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  get bigtext(): string {
    let bigtext = this.deviceService.TranslateStatusShort(this.item.Status);
    if (this.item.SwitchType === 'Selector' || this.item.SubType === 'Evohome') {
      bigtext = Utils.GetLightStatusText(this.item);
    }
    return bigtext;
  }

  get image(): string {
    if (this.item.SwitchType === 'X10 Siren') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status === 'All On')
      ) {
        return 'images/siren-on.png';
      } else {
        return 'images/siren-off.png';
      }
    } else if (this.item.SwitchType === 'Doorbell') {
      return 'images/doorbell48.png';
    } else if (this.item.SwitchType === 'Push On Button') {
      return 'images/' + this.item.Image + '48_On.png';
    } else if (this.item.SwitchType === 'Push Off Button') {
      return 'images/' + this.item.Image + '48_Off.png';
    } else if (this.item.SwitchType === 'Door Contact') {
      if (this.item.InternalState === 'Open') {
        return 'images/' + this.item.Image + '48_On.png';
      } else {
        return 'images/' + this.item.Image + '48_Off.png';
      }
    } else if (this.item.SwitchType === 'Door Lock') {
      if (this.item.InternalState === 'Unlocked') {
        return 'images/' + this.item.Image + '48_On.png';
      } else {
        return 'images/' + this.item.Image + '48_Off.png';
      }
    } else if (this.item.SwitchType === 'Door Lock Inverted') {
      if (this.item.InternalState === 'Unlocked') {
        return 'images/' + this.item.Image + '48_On.png';
      } else {
        return 'images/' + this.item.Image + '48_Off.png';
      }
    } else if (this.item.SwitchType === 'Contact') {
      if (this.item.Status === 'Closed') {
        return 'images/' + this.item.Image + '48_Off.png';
      } else {
        return 'images/' + this.item.Image + '48_On.png';
      }
    } else if (this.item.SwitchType === 'Media Player') {
      if (this.item.CustomImage === 0) {
        this.item.Image = this.item.TypeImg;
      }
      if (this.item.Status === 'Disconnected') {
        return 'images/' + this.item.Image + '48_Off.png';
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        return 'images/' + this.item.Image + '48_On.png';
      } else {
        return 'images/' + this.item.Image + '48_Off.png';
      }
    } else if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        if (this.item.Status === 'Closed') {
          return 'images/blindsopen48.png';
        } else {
          return 'images/blindsopen48sel.png';
        }
      } else {
        if (this.item.Status === 'Closed') {
          return 'images/blindsopen48.png';
        } else {
          return 'images/blindsopen48sel.png';
        }
      }
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      if (this.item.Status === 'Closed') {
        return 'images/blindsopen48.png';
      } else {
        return 'images/blindsopen48sel.png';
      }
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      if (this.item.Status === 'Open') {
        return 'images/blindsopen48sel.png';
      } else {
        return 'images/blindsopen48.png';
      }
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      if (this.item.Status === 'Closed') {
        return 'images/blindsopen48.png';
      } else {
        return 'images/blindsopen48sel.png';
      }
    } else if (this.item.SwitchType === 'Smoke Detector') {
      if (
        (this.item.Status === 'Panic') ||
        (this.item.Status === 'On')
      ) {
        return 'images/smoke48on.png';
      } else {
        return 'images/smoke48off.png';
      }
    } else if (this.item.Type === 'Security') {
      if (this.item.SubType.indexOf('remote') > 0) {
        return 'images/remote48.png';
      } else if (this.item.SubType === 'X10 security') {
        if (this.item.Status.indexOf('Normal') >= 0) {
          return 'images/security48.png';
        } else {
          return 'images/Alarm48_On.png';
        }
      } else if (this.item.SubType === 'X10 security motion') {
        if ((this.item.Status === 'No Motion')) {
          return 'images/security48.png';
        } else {
          return 'images/Alarm48_On.png';
        }
      } else if ((this.item.Status.indexOf('Alarm') >= 0) || (this.item.Status.indexOf('Tamper') >= 0)) {
        return 'images/Alarm48_On.png';
      } else {
        if (this.item.SubType.indexOf('Meiantech') >= 0) {
          return 'images/security48.png';
        } else {
          if (this.item.SubType.indexOf('KeeLoq') >= 0) {
            return 'images/pushon48.png';
          } else {
            return 'images/security48.png';
          }
        }
      }
    } else if (this.item.SwitchType === 'Dimmer') {
      if (this.item.CustomImage === 0) {
        this.item.Image = this.item.TypeImg;
      }
      this.item.Image = this.item.Image.charAt(0).toUpperCase() + this.item.Image.slice(1);
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Set ') === 0) ||
        (this.item.Status.indexOf('NightMode') === 0) ||
        (this.item.Status.indexOf('Disco ') === 0)
      ) {
        if (Utils.isLED(this.item.SubType)) {
          return 'images/RGB48_On.png';
        } else {
          return 'images/' + this.item.Image + '48_On.png';
        }
      } else {
        if (Utils.isLED(this.item.SubType)) {
          return 'images/RGB48_Off.png';
        } else {
          return 'images/' + this.item.Image + '48_Off.png';
        }
      }
    } else if (this.item.SwitchType === 'TPI') {
      if (this.item.Status !== 'Off') {
        return 'images/Fireplace48_On.png';
      } else {
        return 'images/Fireplace48_Off.png';
      }
    } else if (this.item.SwitchType === 'Dusk Sensor') {
      if (this.item.Status === 'On') {
        return 'images/uvdark.png';
      } else {
        return 'images/uvsunny.png';
      }
    } else if (this.item.SwitchType === 'Motion Sensor') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Set ') === 0)
      ) {
        return 'images/motion48-on.png';
      } else {
        return 'images/motion48-off.png';
      }
    } else if (this.item.SwitchType === 'Selector') {
      if (this.item.Status === 'Off') {
        return 'images/' + this.item.Image + '48_Off.png';
      } else if (this.item.LevelOffHidden) {
        return 'images/' + this.item.Image + '48_On.png';
      } else {
        return 'images/' + this.item.Image + '48_On.png';
      }
    } else if (this.item.SubType.indexOf('Itho') === 0) {
      return 'images/Fan48_On.png';
    } else if (this.item.SubType.indexOf('Lucci Air DC') === 0) {
      return 'images/Fan48_On.png';
    } else if (
      (this.item.SubType.indexOf('Lucci') === 0) ||
      (this.item.SubType.indexOf('Westinghouse') === 0)
    ) {
      return 'images/Fan48_On.png';
    } else {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Down') !== -1) ||
        (this.item.Status.indexOf('Up') !== -1) ||
        (this.item.Status.indexOf('Set ') === 0)
      ) {
        return 'images/' + this.item.Image + '48_On.png';
      } else {
        return 'images/' + this.item.Image + '48_Off.png';
      }
    }
  }

  get imageTitle(): string {
    if (this.item.SwitchType === 'X10 Siren') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status === 'All On')
      ) {
        return 'Turn Off';
      } else {
        return 'Turn On';
      }
    } else if (this.item.SwitchType === 'Doorbell') {
      return 'Turn On';
    } else if (this.item.SwitchType === 'Push On Button') {
      return 'Turn On';
    } else if (this.item.SwitchType === 'Push Off Button') {
      return 'Turn Off';
    } else if (this.item.SwitchType === 'Door Contact') {
      if (this.item.InternalState === 'Open') {
        return this.item.InternalState;
      } else {
        return this.item.InternalState;
      }
    } else if (this.item.SwitchType === 'Door Lock') {
      if (this.item.InternalState === 'Unlocked') {
        return 'Lock';
      } else {
        return 'Unlock';
      }
    } else if (this.item.SwitchType === 'Door Lock Inverted') {
      if (this.item.InternalState === 'Unlocked') {
        return 'Lock';
      } else {
        return 'Unlock';
      }
    } else if (this.item.SwitchType === 'Contact') {
      return '';
    } else if (this.item.SwitchType === 'Media Player') {
      return '';
    } else if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      return 'Open Blinds';
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      return 'Open Blinds';
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      return 'Open Blinds';
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      return 'Open Blinds';
    } else if (this.item.SwitchType === 'Smoke Detector') {
      return 'Turn Alarm On';
    } else if (this.item.Type === 'Security') {
      if (this.item.SubType.indexOf('remote') > 0) {
        if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
          return 'Turn Alarm Off';
        } else {
          return 'Turn Alarm On';
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
      } else {
        if (this.item.SubType.indexOf('Meiantech') >= 0) {
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
    } else if (this.item.SwitchType === 'Dimmer') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Set ') === 0) ||
        (this.item.Status.indexOf('NightMode') === 0) ||
        (this.item.Status.indexOf('Disco ') === 0)
      ) {
        if (Utils.isLED(this.item.SubType)) {
        } else {
          return 'Turn Off';
        }
      } else {
        if (Utils.isLED(this.item.SubType)) {
        } else {
          return 'Turn On';
        }
      }
    } else if (this.item.SwitchType === 'TPI') {
      const RO = (this.item.Unit < 64 || this.item.Unit > 95) ? true : false;
      if (this.item.Status !== 'Off') {
        return RO ? 'On' : 'Turn Off';
      } else {
        return RO ? 'Off' : 'Turn On';
      }
    } else if (this.item.SwitchType === 'Dusk Sensor') {
      if (this.item.Status === 'On') {
        return 'Nighttime';
      } else {
        return 'Daytime';
      }
    } else if (this.item.SwitchType === 'Selector') {
      if (this.item.Status === 'Off') {
      } else if (this.item.LevelOffHidden) {
      } else {
        return 'Turn Off';
      }
    } else {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Down') !== -1) ||
        (this.item.Status.indexOf('Up') !== -1) ||
        (this.item.Status.indexOf('Set ') === 0)
      ) {
        if (this.item.Type === 'Thermostat 3') {
        } else {
          return 'Turn Off';
        }
      } else {
        if (this.item.Type === 'Thermostat 3') {
        } else {
          return 'Turn On';
        }
      }
    }
    return '';
  }

  public onImageClick(event: any) {
    if (this.item.SwitchType === 'X10 Siren') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status === 'All On')
      ) {
        this.SwitchLight('Off');
      } else {
        this.SwitchLight('On');
      }
    } else if (this.item.SwitchType === 'Doorbell') {
      this.SwitchLight('On');
    } else if (this.item.SwitchType === 'Push On Button') {
      this.SwitchLight('On');
    } else if (this.item.SwitchType === 'Push Off Button') {
      this.SwitchLight('Off');
    } else if (this.item.SwitchType === 'Door Contact') {
      // Nothing
    } else if (this.item.SwitchType === 'Door Lock') {
      if (this.item.InternalState === 'Unlocked') {
        this.SwitchLight('On');
      } else {
        this.SwitchLight('Off');
      }
    } else if (this.item.SwitchType === 'Door Lock Inverted') {
      if (this.item.InternalState === 'Unlocked') {
        this.SwitchLight('Off');
      } else {
        this.SwitchLight('On');
      }
    } else if (this.item.SwitchType === 'Contact') {
      // Nothing
    } else if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
        // Nothing
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        this.SwitchLight('Off');
      } else {
        this.SwitchLight('On');
      }
    } else if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      this.SwitchLight('Off');
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      this.SwitchLight('On');
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      this.SwitchLight('Off');
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      this.SwitchLight('On');
    } else if (this.item.SwitchType === 'Smoke Detector') {
      this.SwitchLight('On');
    } else if (this.item.Type === 'Security') {
      if (this.item.SubType.indexOf('remote') > 0) {
        if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
          this.SwitchLight('Off');
        } else {
          this.ArmSystem();
        }
      } else if (this.item.SubType === 'X10 security') {
        if (this.item.Status.indexOf('Normal') >= 0) {
          this.SwitchLight(((this.item.Status === 'Normal Delayed') ? 'Alarm Delayed' : 'Alarm'));
        } else {
          this.SwitchLight(((this.item.Status === 'Alarm Delayed') ? 'Normal Delayed' : 'Normal'));
        }
      } else if (this.item.SubType === 'X10 security motion') {
        if ((this.item.Status === 'No Motion')) {
          this.SwitchLight('Motion');
        } else {
          this.SwitchLight('No Motion');
        }
      } else if ((this.item.Status.indexOf('Alarm') >= 0) || (this.item.Status.indexOf('Tamper') >= 0)) {
        // Nothing
      } else {
        if (this.item.SubType.indexOf('Meiantech') >= 0) {
          if ((this.item.Status.indexOf('Arm') >= 0) || (this.item.Status.indexOf('Panic') >= 0)) {
            this.SwitchLight('Off');
          } else {
            this.ArmSystemMeiantech();
          }
        } else {
          if (this.item.SubType.indexOf('KeeLoq') >= 0) {
            this.SwitchLight('On');
          } else {
            // Nothing
          }
        }
      }
    } else if (this.item.SwitchType === 'Dimmer') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Set ') === 0) ||
        (this.item.Status.indexOf('NightMode') === 0) ||
        (this.item.Status.indexOf('Disco ') === 0)
      ) {
        if (Utils.isLED(this.item.SubType)) {
          this.ShowRGBWPopup(event);
        } else {
          this.SwitchLight('Off');
        }
      } else {
        if (Utils.isLED(this.item.SubType)) {
          this.ShowRGBWPopup(event);
        } else {
          this.SwitchLight('On');
        }
      }
    } else if (this.item.SwitchType === 'TPI') {
      const RO = (this.item.Unit < 64 || this.item.Unit > 95) ? true : false;
      if (!RO) {
        if (this.item.Status !== 'Off') {
          this.SwitchLight('Off');
        } else {
          this.SwitchLight('On');
        }
      }
    } else if (this.item.SwitchType === 'Selector') {
      if (this.item.Status === 'Off') {
      } else if (this.item.LevelOffHidden) {
      } else {
        this.SwitchLight('Off');
      }
    } else if (this.item.SubType.indexOf('Itho') === 0) {
      this.ShowIthoPopup(event);
    } else if (this.item.SubType.indexOf('Lucci Air DC') === 0) {
      this.ShowLucciPopup(event);
    } else if (
      (this.item.SubType.indexOf('Lucci') === 0) ||
      (this.item.SubType.indexOf('Westinghouse') === 0)
    ) {
      this.ShowLucciPopup(event);
    } else {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Down') !== -1) ||
        (this.item.Status.indexOf('Up') !== -1) ||
        (this.item.Status.indexOf('Set ') === 0)
      ) {
        if (this.item.Type === 'Thermostat 3') {
          this.ShowTherm3Popup(event);
        } else {
          this.SwitchLight('Off');
        }
      } else {
        if (this.item.Type === 'Thermostat 3') {
          this.ShowTherm3Popup(event);
        } else {
          this.SwitchLight('On');
        }
      }
    }
  }

  get imageCursor(): boolean {
    if (this.item.SwitchType === 'Door Contact') {
      return false;
    } else if (this.item.SwitchType === 'Contact') {
      return false;
    } else if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
        return false;
      }
    } else if (this.item.Type === 'Security') {
      if ((this.item.Status.indexOf('Alarm') >= 0) || (this.item.Status.indexOf('Tamper') >= 0)) {
        return false;
      } else {
        if (this.item.SubType.indexOf('Meiantech') >= 0) {
        } else {
          if (this.item.SubType.indexOf('KeeLoq') >= 0) {
          } else {
            return false;
          }
        }
      }
    } else if (this.item.SwitchType === 'TPI') {
      const RO = (this.item.Unit < 64 || this.item.Unit > 95) ? true : false;
      return !RO;
    } else if (this.item.SwitchType === 'Dusk Sensor') {
      return false;
    } else if (this.item.SwitchType === 'Motion Sensor') {
      return false;
    } else if (this.item.SwitchType === 'Selector') {
      if (this.item.Status === 'Off') {
        return false;
      } else if (this.item.LevelOffHidden) {
        return false;
      }
    }
    return true;
  }

  get image2(): string | null {
    if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
        return 'images/remote48.png';
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        return 'images/remote48.png';
      } else {
        return 'images/remote48.png';
      }
    } else if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        return 'images/blindsstop.png';
      } else {
        if (this.item.Status === 'Closed') {
          return 'images/blinds48sel.png';
        } else {
          return 'images/blinds48.png';
        }
      }
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0)
      ) {
        return 'images/blindsstop.png';
      } else {
        if (this.item.Status === 'Closed') {
          return 'images/blinds48sel.png';
        } else {
          return 'images/blinds48.png';
        }
      }
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      if (this.item.Status === 'Open') {
        return 'images/blinds48.png';
      } else {
        return 'images/blinds48sel.png';
      }
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      if (this.item.Status === 'Closed') {
        return 'images/blinds48sel.png';
      } else {
        return 'images/blinds48.png';
      }
    }
    return null;
  }

  get image2style(): string {
    if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
        return 'opacity:0.4';
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
      } else {
        return 'opacity:0.4';
      }
    }
    return '';
  }

  public onImage2Click(event: any) {
    if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        this.ShowMediaRemote();
      } else {
      }
    } else if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        this.SwitchLight('Stop');
      } else {
        this.SwitchLight('On');
      }
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0)
      ) {
        this.SwitchLight('Stop');
      } else {
        this.SwitchLight('Off');
      }
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      this.SwitchLight('On');
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      this.SwitchLight('Off');
    }
  }

  get image2Title(): string {
    if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        return 'Stop Blinds';
      } else {
        return 'Close Blinds';
      }
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0)
      ) {
        return 'Stop Blinds';
      } else {
        return 'Close Blinds';
      }
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      return 'Close Blinds';
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      return 'Close Blinds';
    }
    return '';
  }

  get image2Cursor(): boolean {
    if (this.item.SwitchType === 'Media Player') {
      if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        return true;
      }
    } else if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      return true;
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      return true;
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      return true;
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      return true;
    }
    return false;
  }

  get image3(): string | null {
    if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        if (this.item.Status === 'Closed') {
          return 'images/blinds48sel.png';
        } else {
          return 'images/blinds48.png';
        }
      }
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0)
      ) {
        if (this.item.Status === 'Closed') {
          return 'images/blinds48sel.png';
        } else {
          return 'images/blinds48.png';
        }
      }
    }
    return null;
  }

  public onImage3Click(event: any) {
    if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        this.SwitchLight('On');
      }
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0)
      ) {
        this.SwitchLight('Off');
      }
    }
  }

  get image3Title(): string {
    if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      if (
        (this.item.SubType === 'RAEX') ||
        (this.item.SubType.indexOf('A-OK') === 0) ||
        (this.item.SubType.indexOf('Hasta') >= 0) ||
        (this.item.SubType.indexOf('Media Mount') === 0) ||
        (this.item.SubType.indexOf('Forest') === 0) ||
        (this.item.SubType.indexOf('Chamberlain') === 0) ||
        (this.item.SubType.indexOf('Sunpery') === 0) ||
        (this.item.SubType.indexOf('Dolat') === 0) ||
        (this.item.SubType.indexOf('ASP') === 0) ||
        (this.item.SubType === 'Harrison') ||
        (this.item.SubType.indexOf('RFY') === 0) ||
        (this.item.SubType.indexOf('ASA') === 0) ||
        (this.item.SubType.indexOf('DC106') === 0) ||
        (this.item.SubType.indexOf('Confexx') === 0) ||
        (this.item.SwitchType.indexOf('Venetian Blinds') === 0)
      ) {
        return 'Close Blinds';
      }
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      return 'Close Blinds';
    }
    return '';
  }

  get image3Cursor(): boolean {
    return true;
  }

  get status(): string {
    if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status.length === 1) {
        this.item.Status = '';
      }
      return this.item.Data;
    }
    return '';
  }

  isLED(): boolean {
    return Utils.isLED(this.item.SubType);
  }

  get levelNames(): Array<string> {
    return Utils.b64DecodeUnicode(this.item.LevelNames).split('|');
  }

  public SwitchSelectorLevel(levelName: string, levelValue: number) {
    this.lightSwitchesService.SwitchSelectorLevel(this.item.idx, levelName, levelValue.toString(), this.item.Protected)
      .pipe(
        mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
      )
      .subscribe((updatedItem) => {
        this.item = updatedItem;
      });
  }

  MakeFavorite(isfavorite: number): void {
    this.deviceService.makeFavorite(Number(this.item.idx), isfavorite).subscribe(_ => {
      this.item.Favorite = isfavorite;
    });
  }

  get addTimer(): boolean {
    if (this.item.SwitchType === 'Doorbell') {
      return false;
    } else if (this.item.SwitchType === 'Door Contact') {
      return false;
    } else if (this.item.SwitchType === 'Door Lock') {
      return false;
    } else if (this.item.SwitchType === 'Door Lock Inverted') {
      return false;
    } else if (this.item.SwitchType === 'Contact') {
      return false;
    } else if (this.item.SwitchType === 'Media Player') {
      return false;
    } else if (this.item.Type === 'Security') {
      return false;
    } else if (this.item.SwitchType === 'Dusk Sensor') {
      return false;
    } else if (this.item.SwitchType === 'Motion Sensor') {
      return false;
    } else if (this.item.SubType.indexOf('Itho') === 0) {
      return false;
    } else if (this.item.SubType.indexOf('Lucci Air DC') === 0) {
      return false;
    } else if (
      (this.item.SubType.indexOf('Lucci') === 0) ||
      (this.item.SubType.indexOf('Westinghouse') === 0)
    ) {
      return false;
    }
    return true;
  }

  // FIXME remove jquery stuff
  public EvohomeImgClick() {
    if (this.evohomeSelected) {
      this.deselect('#evopop_' + this.item.idx);
    } else {
      this.evohomeSelected = true;
      $('#evopop_' + this.item.idx).animate({opacity: 'toggle', height: 'toggle'}, 'fast');
    }
  }

  private deselect(id) {
    const _this = this;
    $(id).animate({opacity: 'toggle', height: 'toggle'}, 'fast', 'swing', function () {
      _this.evohomeSelected = false;
    });
  }

  public EvoPopClick(obj: { name: string, data: string }) {
    this.lightSwitchesService.SwitchModal(this.item.idx, obj.name, obj.data).pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe((updatedItem) => {
      this.item = updatedItem;
      this.deselect('#evopop_' + this.item.idx);
    });
  }

  public ResetSecurityStatus() {
    this.lightSwitchesService.ResetSecurityStatus(this.item.idx, 'Normal').pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe(updatedItem => {
      this.item = updatedItem;
    });
  }

  private SwitchLight(status: string) {
    this.lightSwitchesService.SwitchLight(this.item.idx, status, this.item.Protected).pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe(updatedItem => {
      this.item = updatedItem;
    });
  }

  private ShowMediaRemote() {
    this.domoticzDevicesService.ShowMediaRemote(this.item.Name, Number(this.item.idx), this.item.HardwareType);
  }

  ArmSystem() {
    this.lightSwitchesService.ArmSystem(this.item.idx, this.item.Protected).pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe(updatedItem => {
      this.item = updatedItem;
    });
  }

  ArmSystemMeiantech() {
    this.lightSwitchesService.ArmSystemMeiantech(this.item.idx, this.item.Protected).pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe(updatedItem => {
      this.item = updatedItem;
    });
  }

  ShowRGBWPopup(event: Event) {
    this.rgbwPopup.ShowRGBWPopup(event);
  }

  ShowLucciPopup(event: Event) {
    this.lucciPopup.ShowLucciPopup(event);
  }

  ShowIthoPopup(event: Event) {
    this.ithoPopup.ShowIthoPopup(event);
  }

  ShowTherm3Popup(event: Event) {
    this.therm3Popup.ShowTherm3Popup(event);
  }

  ShowCameraLiveStream() {
    this.domoticzDevicesService.ShowCameraLiveStream(this.item.Name, this.item.CameraIdx);
  }

  ShowRFYPopup(event: any) {
    this.rfyPopup.ShowRFYPopup(event, this.item.idx, this.item.Protected, this.configService.globals.ismobile);
  }

  refreshDevice() {
    this.deviceService.getDeviceInfo(this.item.idx).subscribe(device => {
      this.item = device;
    });
  }

}
