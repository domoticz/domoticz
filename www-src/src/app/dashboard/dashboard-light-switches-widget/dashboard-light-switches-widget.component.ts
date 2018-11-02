import {AfterViewInit, Component, Input, OnInit, ViewChild} from '@angular/core';
import {LightSwitchesService} from 'src/app/_shared/_services/light-switches.service';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {ConfigService} from '../../_shared/_services/config.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {DialogService} from '../../_shared/_services/dialog.service';
import {mergeMap} from 'rxjs/operators';
import {Router} from '@angular/router';
import {RgbwPopupComponent} from 'src/app/_shared/_components/rgbw-popup/rgbw-popup.component';
import {LucciPopupComponent} from 'src/app/_shared/_components/lucci-popup/lucci-popup.component';
import {IthoPopupComponent} from 'src/app/_shared/_components/itho-popup/itho-popup.component';
import {Therm3PopupComponent} from 'src/app/_shared/_components/therm3-popup/therm3-popup.component';
import {DimmerSliderComponent} from 'src/app/_shared/_components/dimmer-slider/dimmer-slider.component';
import {SelectorlevelsComponent} from 'src/app/_shared/_components/selectorlevels/selectorlevels.component';
import {Device} from '../../_shared/_models/device';
import {DomoticzDevicesService} from '../../_shared/_services/domoticz-devices.service';

@Component({
  selector: 'dz-dashboard-light-switches-widget',
  templateUrl: './dashboard-light-switches-widget.component.html',
  styleUrls: ['./dashboard-light-switches-widget.component.css']
})
export class DashboardLightSwitchesWidgetComponent implements OnInit, AfterViewInit {

  @Input() item: Device;

  @ViewChild(RgbwPopupComponent, {static: false}) rgbwPopup?: RgbwPopupComponent;
  @ViewChild(LucciPopupComponent, {static: false}) lucciPopup?: LucciPopupComponent;
  @ViewChild(IthoPopupComponent, {static: false}) ithoPopup?: IthoPopupComponent;
  @ViewChild(Therm3PopupComponent, {static: false}) therm3Popup?: Therm3PopupComponent;

  @ViewChild(DimmerSliderComponent, {static: false}) dimmerSlider?: DimmerSliderComponent;
  @ViewChild(SelectorlevelsComponent, {static: false}) selectorlevels?: SelectorlevelsComponent;

  constructor(
    private lightSwitchesService: LightSwitchesService,
    private deviceService: DeviceService,
    private domoticzDevicesService: DomoticzDevicesService,
    private configService: ConfigService,
    private dialogService: DialogService,
    private router: Router
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    if (this.dimmerSlider) {
      this.dimmerSlider.ResizeDimSliders();
    }
    if (this.selectorlevels) {
      this.selectorlevels.Resize();
    }
  }

  get backgroundClass(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  get tableClass(): string {
    if ((this.item.Type.indexOf('Blind') === 0) || (this.item.SwitchType === 'Blinds') ||
      (this.item.SwitchType === 'Blinds Inverted') || (this.item.SwitchType === 'Blinds Percentage') ||
      (this.item.SwitchType === 'Blinds Percentage Inverted') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0) ||
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
        return 'itemtablesmalltrippleicon';
      } else {
        return 'itemtablesmalldoubleicon';
      }
    } else {
      return 'itemtablesmall';
    }
  }

  get bigtext(): string {
    if (this.item.SwitchType === 'Selector') {
      return Utils.GetLightStatusText(this.item);
    } else {
      return this.deviceService.TranslateStatusShort(this.item.Status);
    }
  }

  ShowCameraLiveStream() {
    this.domoticzDevicesService.ShowCameraLiveStream(this.item.Name, this.item.CameraIdx);
  }

  get status(): string {
    let status = '';
    if (this.item.SwitchType === 'Media Player') {
      status = this.item.Data;
    }
    return status;
  }

  get img1Src(): string {
    if (this.item.SwitchType === 'Doorbell') {
      return 'doorbell48.png';
    } else if (this.item.SwitchType === 'Push On Button') {
      return this.item.Image + '48_On.png';
    } else if (this.item.SwitchType === 'Push Off Button') {
      return this.item.Image + '48_Off.png';
    } else if (this.item.SwitchType === 'Door Contact') {
      if (this.item.InternalState === 'Open') {
        return this.item.Image + '48_On.png';
      } else {
        return this.item.Image + '48_Off.png';
      }
    } else if (this.item.SwitchType === 'Door Lock') {
      if (this.item.InternalState === 'Unlocked') {
        return this.item.Image + '48_On.png';
      } else {
        return this.item.Image + '48_Off.png';
      }
    } else if (this.item.SwitchType === 'Door Lock Inverted') {
      if (this.item.InternalState === 'Unlocked') {
        return this.item.Image + '48_On.png';
      } else {
        return this.item.Image + '48_Off.png';
      }
    } else if (this.item.SwitchType === 'X10 Siren') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status === 'All On')
      ) {
        return 'siren-on.png';
      } else {
        return 'siren-off.png';
      }
    } else if (this.item.SwitchType === 'Contact') {
      if (this.item.Status === 'Closed') {
        return this.item.Image + '48_Off.png';
      } else {
        return this.item.Image + '48_On.png';
      }
    } else if (this.item.SwitchType === 'Media Player') {
      if (this.item.CustomImage === 0) {
        this.item.Image = this.item.TypeImg;
      }
      if (this.item.Status === 'Disconnected') {
        return this.item.Image + '48_Off.png';
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        return this.item.Image + '48_On.png';
      } else {
        return this.item.Image + '48_Off.png';
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
          return 'blindsopen48.png';
        } else {
          return 'blindsopen48sel.png';
        }
      } else {
        if (this.item.Status === 'Closed') {
          return 'blindsopen48.png';
        } else {
          return 'blindsopen48sel.png';
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
          return 'blindsopen48.png';
        } else {
          return 'blindsopen48sel.png';
        }
      } else {
        if (this.item.Status === 'Closed') {
          return 'blindsopen48.png';
        } else {
          return 'blindsopen48sel.png';
        }
      }
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      if (this.item.Status === 'Open') {
        return 'blindsopen48sel.png';
      } else {
        return 'blindsopen48.png';
      }
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      if (this.item.Status === 'Closed') {
        return 'blindsopen48.png';
      } else {
        return 'blindsopen48sel.png';
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
          return 'RGB48_On.png';
        } else {
          return this.item.Image + '48_On.png';
        }
      } else {
        if (Utils.isLED(this.item.SubType)) {
          return 'RGB48_Off.png';
        } else {
          return this.item.Image + '48_Off.png';
        }
      }
    } else if (this.item.SwitchType === 'TPI') {
      const RO = (this.item.Unit < 64 || this.item.Unit > 95) ? true : false;
      if (this.item.Status !== 'Off') {
        return 'Fireplace48_On.png';
      } else {
        return 'Fireplace48_Off.png';
      }
    } else if (this.item.SwitchType === 'Dusk Sensor') {
      if (this.item.Status === 'On') {
        return 'uvdark.png';
      } else {
        return 'uvsunny.png';
      }
    } else if (this.item.SwitchType === 'Motion Sensor') {
      if (
        (this.item.Status === 'On') ||
        (this.item.Status === 'Chime') ||
        (this.item.Status === 'Group On') ||
        (this.item.Status.indexOf('Set ') === 0)
      ) {
        return 'motion48-on.png';
      } else {
        return 'motion48-off.png';
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
    } else if (this.item.SwitchType === 'Selector') {
      if (this.item.Status === 'Off') {
        return this.item.Image + '48_Off.png';
      } else if (this.item.LevelOffHidden) {
        return this.item.Image + '48_On.png';
      } else {
        return this.item.Image + '48_On.png';
      }
    } else if (this.item.SubType.indexOf('Itho') === 0) {
      return 'Fan48_On.png';
    } else if (this.item.SubType.indexOf('Lucci Air DC') === 0) {
      return 'Fan48_On.png';
    } else if (
      (this.item.SubType.indexOf('Lucci') === 0) ||
      (this.item.SubType.indexOf('Westinghouse') === 0)
    ) {
      return 'Fan48_On.png';
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
          return this.item.Image + '48_On.png';
        } else {
          return this.item.Image + '48_On.png';
        }
      } else {
        if (this.item.Type === 'Thermostat 3') {
          return this.item.Image + '48_Off.png';
        } else {
          return this.item.Image + '48_Off.png';
        }
      }
    }
  }

  get img1Title(): string {
    if (this.item.SwitchType === 'Doorbell') {
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
    } else if (this.item.SwitchType === 'X10 Siren') {
      return '';
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
    } else if (this.item.SwitchType === 'Dimmer') {
      if (Utils.isLED(this.item.SubType)) {
        return '';
      } else {
        return 'Turn Off';
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
    } else if (this.item.SwitchType === 'Motion Sensor') {
      return '';
    } else if (this.item.SwitchType === 'Smoke Detector') {
      return '';
    } else if (this.item.SwitchType === 'Selector') {
      return '';
    } else if (this.item.SubType.indexOf('Itho') === 0) {
      return '';
    } else if (this.item.SubType.indexOf('Lucci Air DC') === 0) {
      return '';
    } else if (
      (this.item.SubType.indexOf('Lucci') === 0) ||
      (this.item.SubType.indexOf('Westinghouse') === 0)
    ) {
      return '';
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
          return '';
        } else {
          return 'Turn Off';
        }
      } else {
        if (this.item.Type === 'Thermostat 3') {
          return '';
        } else {
          return 'Turn On';
        }
      }
    }
  }

  get img1Class(): string {
    let klass = 'lcursor';
    if (this.item.SwitchType === 'Door Contact') {
      klass = '';
    } else if (this.item.SwitchType === 'X10 Siren') {
      klass = '';
    } else if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
        klass = '';
      }
    } else if (this.item.SwitchType === 'TPI') {
      const RO = (this.item.Unit < 64 || this.item.Unit > 95) ? true : false;
      klass = RO ? '' : 'lcursor';
    } else if (this.item.SwitchType === 'Selector') {
      if (this.item.Status === 'Off') {
        klass = '';
      } else if (this.item.LevelOffHidden) {
        klass = '';
      }
    }
    return klass;
  }

  onImg1Click(event: any) {
    if (this.item.SwitchType === 'Doorbell') {
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
    } else if (this.item.SwitchType === 'X10 Siren') {
      // Nothing
    } else if (this.item.SwitchType === 'Contact') {
      this.router.navigate(['/Devices', this.item.idx, 'Log']);
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
    } else if (this.item.SwitchType === 'Dusk Sensor') {
      this.router.navigate(['/Devices', this.item.idx, 'Log']);
    } else if (this.item.SwitchType === 'Motion Sensor') {
      this.router.navigate(['/Devices', this.item.idx, 'Log']);
    } else if (this.item.SwitchType === 'Smoke Detector') {
      this.router.navigate(['/Devices', this.item.idx, 'Log']);
    } else if (this.item.SwitchType === 'Selector') {
      if (this.item.Status === 'Off') {
        // Nothing
      } else if (this.item.LevelOffHidden) {
        // Nothing
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

  private SwitchLight(status: string) {
    this.lightSwitchesService.SwitchLight(this.item.idx, status, this.item.Protected).pipe(
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

  refreshDevice() {
    this.deviceService.getDeviceInfo(this.item.idx).subscribe(device => {
      this.item = device;
    });
  }

  get img2Src(): string {
    if (this.item.SwitchType === 'Media Player') {
      return 'remote48.png';
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
        return 'blindsstop.png';
      } else {
        if (this.item.Status === 'Closed') {
          return 'blinds48sel.png';
        } else {
          return 'blinds48.png';
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
        return 'blindsstop.png';
      } else {
        if (this.item.Status === 'Closed') {
          return 'blinds48sel.png';
        } else {
          return 'blinds48.png';
        }
      }
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      if (this.item.Status === 'Open') {
        return 'blinds48.png';
      } else {
        return 'blinds48sel.png';
      }
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      if (this.item.Status === 'Closed') {
        return 'blinds48sel.png';
      } else {
        return 'blinds48.png';
      }
    } else {
      return undefined;
    }
  }

  get img2Style(): string {
    if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
        return 'opacity:0.4;';
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        return '';
      } else {
        return 'opacity:0.4;';
      }
    } else {
      return '';
    }
  }

  onImg2Click(event: any) {
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
      if (this.item.Status === 'Open') {
        this.SwitchLight('On');
      } else {
        this.SwitchLight('On');
      }
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      if (this.item.Status === 'Closed') {
        this.SwitchLight('Off');
      } else {
        this.SwitchLight('Off');
      }
    }
  }

  get img2Class(): string {
    if (this.item.SwitchType === 'Media Player') {
      if (this.item.Status === 'Disconnected') {
        return '';
      } else if ((this.item.Status !== 'Off') && (this.item.Status !== '0')) {
        return 'lcursor';
      } else {
        return '';
      }
    } else if ((this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0)) {
      return 'lcursor';
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      return 'lcursor';
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      return 'lcursor';
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      return 'lcursor';
    } else {
      return '';
    }
  }

  get img2Title(): string {
    if (this.item.SwitchType === 'Media Player') {
      return '';
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
    } else {
      return '';
    }
  }

  private ShowMediaRemote() {
    this.domoticzDevicesService.ShowMediaRemote(this.item.Name, Number(this.item.idx), this.item.HardwareType);
  }

  get img3Src(): string {
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
          return 'blinds48sel.png';
        } else {
          return 'blinds48.png';
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
          return 'blinds48sel.png';
        } else {
          return 'blinds48.png';
        }
      }
    }
  }

  get img3Title(): string {
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
        return 'Close Blinds';
      }
    }
  }

  onImg3Click(event: any) {
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

}
