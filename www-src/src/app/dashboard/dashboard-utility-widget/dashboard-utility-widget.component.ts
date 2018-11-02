import { Component, OnInit, Input, Inject, ViewChild } from '@angular/core';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { DeviceService } from 'src/app/_shared/_services/device.service';
import { Utils } from 'src/app/_shared/_utils/utils';
import { SetpointPopupComponent } from 'src/app/_shared/_components/setpoint-popup/setpoint-popup.component';
import {Device} from "../../_shared/_models/device";

@Component({
  selector: 'dz-dashboard-utility-widget',
  templateUrl: './dashboard-utility-widget.component.html',
  styleUrls: ['./dashboard-utility-widget.component.css']
})
export class DashboardUtilityWidgetComponent implements OnInit {

  @Input() item: Device;

  @ViewChild(SetpointPopupComponent, { static: false }) setpointPopup: SetpointPopupComponent;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
  }

  get itemClasses(): string[] {
    /* type of device */
    let itemtypeclass = '';
    let itemsubtypeclass = '';
    if (typeof this.item.Type !== 'undefined') {
      itemtypeclass = ' ' + this.item.Type.slice(0);
      itemtypeclass = itemtypeclass.replace(/\s/g, '');
      itemtypeclass = itemtypeclass.replace(/\\/g, '');
      itemtypeclass = itemtypeclass.replace(/\//g, '');
    }
    if (typeof this.item.SubType !== 'undefined') {
      itemsubtypeclass = this.item.SubType.split(' ').join('');
      itemsubtypeclass = itemsubtypeclass.replace(/\\/g, '');
      itemsubtypeclass = itemsubtypeclass.replace(/\//g, '');
    }

    const backgroundClass = Utils.GetItemBackgroundStatus(this.item);

    /* checking the generated html for even more classes, then fill in the HTML */
    const count = (this.statushtml.match(/<span/g) || []).length; // $(statushtml).find("span").length;

    if (this.statushtml.length !== this.bigtexthtml.length) {
      return ['item', itemtypeclass, itemsubtypeclass, backgroundClass, 'withstatus', 'statuscount' + count];
    } else {
      return ['item', itemtypeclass, itemsubtypeclass, backgroundClass, 'withoutstatus', 'statuscount' + count];
    }
  }

  get bigtexthtml(): string {
    let bigtexthtml = '';
    bigtexthtml += '<span class="value1">';
    if ((typeof this.item.Usage !== 'undefined') && (typeof this.item.UsageDeliv === 'undefined')) {
      bigtexthtml += this.item.Usage;
    } else if ((typeof this.item.Usage !== 'undefined') && (typeof this.item.UsageDeliv !== 'undefined')) {
      if (Number(this.item.Usage) > 0) {
        bigtexthtml += this.item.Usage;
      } else if (Number(this.item.UsageDeliv) > 0) {
        bigtexthtml += '-' + this.item.UsageDeliv;
      } else {
        bigtexthtml += this.item.Usage;
      }
    } else if (
      (this.item.SubType === 'Gas') ||
      (this.item.SubType === 'RFXMeter counter') ||
      (this.item.SubType === 'Counter Incremental')
    ) {
      bigtexthtml += this.item.CounterToday;
    } else if (this.item.SubType === 'Managed Counter') {
      bigtexthtml = this.item.Counter;
    } else if (
      (this.item.Type === 'Air Quality') ||
      (this.item.Type === 'Lux') ||
      (this.item.Type === 'Weight') ||
      (this.item.Type === 'Usage') ||
      (this.item.SubType === 'Percentage') ||
      (this.item.SubType === 'Fan') ||
      (this.item.SubType === 'Soil Moisture') ||
      (this.item.SubType === 'Leaf Wetness') ||
      (this.item.SubType === 'Voltage') ||
      (this.item.SubType === 'Distance') ||
      (this.item.SubType === 'Current') ||
      (this.item.SubType === 'Pressure') ||
      (this.item.SubType === 'A/D') ||
      (this.item.SubType === 'Sound Level') ||
      (this.item.SubType === 'Waterflow') ||
      (this.item.Type === 'Current') ||
      (this.item.SubType === 'Custom Sensor')
    ) {
      bigtexthtml += this.item.Data;
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      bigtexthtml += this.item.Data + '\u00B0 ' + this.configService.config.TempSign;
    } else if (this.item.SubType === 'Smartwares') {
      bigtexthtml += this.item.Data + '\u00B0 ' + this.configService.config.TempSign;
    }
    bigtexthtml += '</span>';

    return bigtexthtml;
  }

  get statushtml(): string {
    let statushtml = '';

    if (typeof this.item.Counter !== 'undefined') {
      if (
        (this.item.SubType === 'RFXMeter counter') ||
        (this.item.SubType === 'Counter Incremental')
      ) {
        statushtml = this.item.Counter;
      } else if (this.item.SubType !== 'Gas' && this.item.SubType !== 'Managed Counter') { // this is weird..
        statushtml = '' + this.translationService.t('Usage') + ': ' + this.item.CounterToday;
      } else if ((this.item.SubType === 'Gas')) {
        // added this to fill the status value. If it's the same as the bigtext, then it won't be shown again.
        statushtml += '';
      } else {
        statushtml = '';
      }
    } else if ((this.item.Type === 'Energy') || (this.item.Type === 'Power') || (this.item.SubType === 'kWh')) {
      statushtml = '';
    } else if ((this.item.Type === 'Current') || (this.item.Type === 'Current/Energy')) {
      statushtml = '';
    } else if (this.item.Type === 'Air Quality') {
      statushtml = this.item.Quality;
    } else if (this.item.SubType === 'Percentage') {
      statushtml = '';
    } else if (this.item.SubType === 'Fan') {
      statushtml = '';
    } else if (this.item.Type === 'Lux') {
      statushtml = '';
    } else if (this.item.Type === 'Weight') {
      statushtml = '';
    } else if (this.item.Type === 'Usage') {
      statushtml = '';
    } else if (this.item.SubType === 'Soil Moisture') {
      statushtml = '';
    } else if (this.item.SubType === 'Custom Sensor') {
      statushtml = '';
    } else if (this.item.SubType === 'Waterflow') {
      statushtml = '';
    } else if (this.item.SubType === 'Leaf Wetness') {
      statushtml = '';
    } else if (this.item.SubType === 'Distance') {
      statushtml = '';
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      statushtml = '';
    } else if (this.item.SubType === 'Text') {
      statushtml = this.item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
    } else if (this.item.SubType === 'Alert') {
      statushtml = this.item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
    } else if (this.item.SubType === 'Pressure') {
      statushtml = '';
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      statushtml = '';
    } else if (this.item.SubType === 'Smartwares') {
      statushtml = this.item.Data + '\u00B0 ' + this.configService.config.TempSign;
    } else if ((this.item.SubType === 'Thermostat Mode') || (this.item.SubType === 'Thermostat Fan Mode')) {
      statushtml = '';
    } else if (this.item.SubType === 'Sound Level') {
      statushtml = '';
    }

    if (typeof this.item.Usage !== 'undefined') {
      if (this.item.Type !== 'P1 Smart Meter') {
        if (this.configService.config.DashboardType === 0) {
          // status+='<br>' + this.translationService.t("Actual") + ': ' + item.Usage;
          if (typeof this.item.CounterToday !== 'undefined') {
            statushtml += this.translationService.t('Today') + ': ' + this.item.CounterToday;
          }
        } else {
          // status+=", A: " + item.Usage;
          if (typeof this.item.CounterToday !== 'undefined') {
            statushtml += 'T: ' + this.item.CounterToday;
          }
        }
      }
    }
    if (typeof this.item.CounterDeliv !== 'undefined') {
      if (this.item.CounterDeliv !== '0') {
        statushtml += '</span><span class="value2">';
        statushtml += '<br>' + this.translationService.t('Return') + ': ' + this.item.CounterDelivToday;
      }
    }
    statushtml = '<span class="value1">' + statushtml + '</span>';

    return statushtml;
  }

  get imageLink(): string[] {
    if (typeof this.item.Counter !== 'undefined') {
      if ((this.item.Type === 'RFXMeter') || (this.item.Type === 'YouLess Meter')
        || (this.item.SubType === 'Counter Incremental') || (this.item.Type === 'Managed Counter')) {
        if (this.item.SwitchTypeVal === 1) {
          return ['/Devices', this.item.idx, 'Log'];
        } else if (this.item.SwitchTypeVal === 2) {
          return ['/Devices', this.item.idx, 'Log'];
        } else if (this.item.SwitchTypeVal === 3) {
          return ['/Devices', this.item.idx, 'Log'];
        } else if (this.item.SwitchTypeVal === 4) {
          return ['/Devices', this.item.idx, 'Log'];
        } else {
          return ['/Devices', this.item.idx, 'Log'];
        }
      } else {
        if ((this.item.Type === 'P1 Smart Meter') && (this.item.SubType === 'Gas')) {
          return ['/Devices', this.item.idx, 'Log'];
        } else {
          return ['/Devices', this.item.idx, 'Log'];
        }
      }
    } else if ((this.item.Type === 'Energy') || (this.item.Type === 'Power') || (this.item.SubType === 'kWh')) {
      if (((this.item.Type === 'Energy') || (this.item.Type === 'Power') || (this.item.SubType === 'kWh'))
        && (this.item.SwitchTypeVal === 4)) {
        return ['/Devices', this.item.idx, 'Log'];
      } else {
        return ['/Devices', this.item.idx, 'Log'];
      }
    } else if ((this.item.Type === 'Current') || (this.item.Type === 'Current/Energy')) {
      return ['/CurrentLog', this.item.idx];
    } else if (this.item.Type === 'Air Quality') {
      return ['/AirQualityLog', this.item.idx];
    } else if (this.item.SubType === 'Percentage') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Fan') {
      return ['/FanLog', this.item.idx];
    } else if (this.item.Type === 'Lux') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Weight') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Usage') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Soil Moisture') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Custom Sensor') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Waterflow') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Leaf Wetness') {
      return [];
    } else if (this.item.SubType === 'Distance') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Text') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Alert') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Pressure') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      return [];
    } else if (this.item.SubType === 'Smartwares') {
      return [];
    } else if ((this.item.SubType === 'Thermostat Mode') || (this.item.SubType === 'Thermostat Fan Mode')) {
      return [];
    } else if (this.item.SubType === 'Sound Level') {
      return ['/Devices', this.item.idx, 'Log'];
    } else {
      return [];
    }
  }

  get imageSrc(): string {
    if (typeof this.item.Counter !== 'undefined') {
      if ((this.item.Type === 'RFXMeter') || (this.item.Type === 'YouLess Meter') ||
        (this.item.SubType === 'Counter Incremental') || (this.item.Type === 'Managed Counter')) {
        if (this.item.SwitchTypeVal === 1) {
          return 'Gas48.png';
        } else if (this.item.SwitchTypeVal === 2) {
          return 'Water48_On.png';
        } else if (this.item.SwitchTypeVal === 3) {
          return 'Counter48.png';
        } else if (this.item.SwitchTypeVal === 4) {
          return 'PV48.png';
        } else {
          return 'Counter48.png';
        }
      } else {
        if ((this.item.Type === 'P1 Smart Meter') && (this.item.SubType === 'Gas')) {
          return 'Gas48.png';
        } else {
          return 'Counter48.png';
        }
      }
    } else if ((this.item.Type === 'Energy') || (this.item.Type === 'Power') || (this.item.SubType === 'kWh')) {
      if (((this.item.Type === 'Energy') || (this.item.Type === 'Power') || (this.item.SubType === 'kWh'))
        && (this.item.SwitchTypeVal === 4)) {
        return 'PV48.png';
      } else {
        return 'current48.png';
      }
    } else if ((this.item.Type === 'Current') || (this.item.Type === 'Current/Energy')) {
      return 'current48.png';
    } else if (this.item.Type === 'Air Quality') {
      return 'air48.png';
    } else if (this.item.SubType === 'Percentage') {
      return 'Percentage48.png';
    } else if (this.item.SubType === 'Fan') {
      return 'Fan48_On.png';
    } else if (this.item.Type === 'Lux') {
      return 'lux48.png';
    } else if (this.item.Type === 'Weight') {
      return 'scale48.png';
    } else if (this.item.Type === 'Usage') {
      return 'current48.png';
    } else if (this.item.SubType === 'Soil Moisture') {
      return 'moisture48.png';
    } else if (this.item.SubType === 'Custom Sensor') {
      return this.item.Image + '48_On.png';
    } else if (this.item.SubType === 'Waterflow') {
      return 'moisture48.png';
    } else if (this.item.SubType === 'Leaf Wetness') {
      return 'leaf48.png';
    } else if (this.item.SubType === 'Distance') {
      return 'visibility48.png';
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      return 'current48.png';
    } else if (this.item.SubType === 'Text') {
      return 'text48.png';
    } else if (this.item.SubType === 'Alert') {
      return 'Alert48_' + this.item.Level + '.png';
    } else if (this.item.SubType === 'Pressure') {
      return 'gauge48.png';
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      return 'override.png';
    } else if (this.item.SubType === 'Smartwares') {
      return 'override.png';
    } else if ((this.item.SubType === 'Thermostat Mode') || (this.item.SubType === 'Thermostat Fan Mode')) {
      return 'mode48.png';
    } else if (this.item.SubType === 'Sound Level') {
      return 'Speaker48_On.png';
    }
  }

  get imageClass(): string {
    if (((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) || (this.item.SubType === 'Smartwares')) {
      return 'lcursor';
    } else {
      return '';
    }
  }

  onImageClick(event: any) {
    if (((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) || (this.item.SubType === 'Smartwares')) {
      this.setpointPopup.ShowSetpointPopup(event, this.item.idx, this.item.Protected, this.item.Data);
    }
  }

  onSetpointSet() {
    this.deviceService.getDeviceInfo(this.item.idx).subscribe(device => {
      this.item = device;
    });
  }

}
