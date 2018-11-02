import {AfterViewInit, Component, Input, OnInit, ViewChild} from '@angular/core';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {LightSwitchesService} from '../../_shared/_services/light-switches.service';
import {mergeMap} from 'rxjs/operators';
import {Utils} from 'src/app/_shared/_utils/utils';
import {DimmerSliderComponent} from 'src/app/_shared/_components/dimmer-slider/dimmer-slider.component';
import {SelectorlevelsComponent} from 'src/app/_shared/_components/selectorlevels/selectorlevels.component';
import {Device} from '../../_shared/_models/device';

@Component({
  selector: '[dzDashboardMobileLightSwitchesWidget]',
  templateUrl: './dashboard-mobile-light-switches-widget.component.html',
  styleUrls: ['./dashboard-mobile-light-switches-widget.component.css']
})
export class DashboardMobileLightSwitchesWidgetComponent implements OnInit, AfterViewInit {

  @Input() item: Device;

  @ViewChild(DimmerSliderComponent, {static: false}) dimmerSlider?: DimmerSliderComponent;
  @ViewChild(SelectorlevelsComponent, {static: false}) selectorlevels?: SelectorlevelsComponent;

  constructor(
    private lightSwitchesService: LightSwitchesService,
    private deviceService: DeviceService
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

  get status(): string {
    return this.deviceService.TranslateStatus(this.item.Status);
  }

  SwitchLight(switchcmd: string) {
    this.lightSwitchesService.SwitchLight(this.item.idx, switchcmd, this.item.Protected).pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe(updatedItem => {
      this.item = updatedItem;
    });
  }

  get sirenOn(): boolean {
    return (this.item.Status === 'On') ||
      (this.item.Status === 'Chime') ||
      (this.item.Status === 'Group On') ||
      (this.item.Status === 'All On');
  }

  get isBlinds(): boolean {
    return (this.item.SwitchType === 'Blinds') || (this.item.SwitchType.indexOf('Venetian Blinds') === 0);
  }

  get isBlinds3States(): boolean {
    return (this.item.SubType === 'RAEX') ||
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
      (this.item.SwitchType.indexOf('Venetian Blinds') === 0);
  }

  get isBlindsInverted3States(): boolean {
    return (this.item.SubType === 'RAEX') ||
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
      (this.item.SubType.indexOf('Confexx') === 0);
  }

  get isDimmerOn(): boolean {
    return (this.item.Status === 'On') ||
      (this.item.Status === 'Chime') ||
      (this.item.Status === 'Group On') ||
      (this.item.Status.indexOf('Set ') === 0);
  }

  get TPIRO(): boolean {
    return (this.item.Unit < 64 || this.item.Unit > 95);
  }

  get isMotionSensorOn(): boolean {
    return (this.item.Status === 'On') ||
      (this.item.Status === 'Chime') ||
      (this.item.Status === 'Group On') ||
      (this.item.Status.indexOf('Set ') === 0);
  }

  get isSmokeDetectorOn(): boolean {
    return (this.item.Status === 'Panic') ||
      (this.item.Status === 'On');
  }

  get default(): boolean {
    if (this.item.SwitchType === 'Doorbell') {
      return false;
    } else if (this.item.SwitchType === 'Push On Button') {
      return false;
    } else if (this.item.SwitchType === 'Push Off Button') {
      return false;
    } else if (this.item.SwitchType === 'Door Contact') {
      return false;
    } else if (this.item.SwitchType === 'Door Lock') {
      return false;
    } else if (this.item.SwitchType === 'Door Lock Inverted') {
      return false;
    } else if (this.item.SwitchType === 'X10 Siren') {
      return false;
    } else if (this.item.SwitchType === 'Contact') {
      return false;
    } else if (this.isBlinds) {
      return false;
    } else if (this.item.SwitchType === 'Blinds Inverted') {
      return false;
    } else if (this.item.SwitchType === 'Blinds Percentage') {
      return false;
    } else if (this.item.SwitchType === 'Blinds Percentage Inverted') {
      return false;
    } else if (this.item.SwitchType === 'Dimmer') {
      return false;
    } else if (this.item.SwitchType === 'TPI') {
      return false;
    } else if (this.item.SwitchType === 'Dusk Sensor') {
      return false;
    } else if (this.item.SwitchType === 'Motion Sensor') {
      return false;
    } else if (this.item.SwitchType === 'Smoke Detector') {
      return false;
    } else if (this.item.SwitchType === 'Selector') {
      return false;
    } else if (this.item.SubType.indexOf('Itho') === 0) {
      return false;
    } else if ((this.item.SubType.indexOf('Lucci') === 0) || (this.item.SubType.indexOf('Westinghouse') === 0)) {
      return false;
    } else if (this.item.SubType.indexOf('Lucci Air DC') === 0) {
      return false;
    } else {
      return true;
    }
  }

  get isDefaultOn(): boolean {
    return (this.item.Status === 'On') ||
      (this.item.Status === 'Chime') ||
      (this.item.Status === 'Group On') ||
      (this.item.Status.indexOf('Down') !== -1) ||
      (this.item.Status.indexOf('Up') !== -1) ||
      (this.item.Status.indexOf('Set ') === 0);
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
