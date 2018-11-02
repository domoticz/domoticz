import { Component, OnInit, Input, Inject, ViewChild } from '@angular/core';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { SetpointPopupComponent } from 'src/app/_shared/_components/setpoint-popup/setpoint-popup.component';
import { DeviceService } from 'src/app/_shared/_services/device.service';
import {Device} from "../../_shared/_models/device";

@Component({
  selector: '[dzDashboardMobileUtilityWidget]',
  templateUrl: './dashboard-mobile-utility-widget.component.html',
  styleUrls: ['./dashboard-mobile-utility-widget.component.css']
})
export class DashboardMobileUtilityWidgetComponent implements OnInit {

  @Input() item: Device;

  @ViewChild(SetpointPopupComponent, { static: false }) setpointPopup: SetpointPopupComponent;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
  }

  get nextLink(): string[] {
    if (typeof this.item.Counter !== 'undefined') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'Current') || (this.item.Type === 'Current/Energy')) {
      return ['/CurrentLog', this.item.idx];
    } else if ((this.item.Type === 'Energy') || (this.item.SubType === 'kWh') || (this.item.SubType === 'Power')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Air Quality') {
      return ['/AirQualityLog', this.item.idx];
    } else if (this.item.SubType === 'Percentage') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Custom Sensor') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Fan') {
      return ['/FanLog', this.item.idx];
    } else if (this.item.Type === 'Lux') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Usage') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Soil Moisture') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Distance') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Text' || this.item.SubType === 'Alert') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Pressure') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Smartwares') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Sound Level') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Waterflow') {
      return ['/Devices', this.item.idx, 'Log'];
    }
  }

  get imageSrc(): string {
    if (this.item.SubType === 'Custom Sensor') {
      return this.item.Image + '48_On.png';
    } else {
      return 'next.png';
    }
  }

  get imageClass(): string {
    if (this.item.SubType === 'Fan') {
      return 'fanicon';
    } else {
      return '';
    }
  }

  get status(): string {
    let status = '';
    if (typeof this.item.Counter !== 'undefined') {
      if (this.configService.config.DashboardType === 0) {
	if (this.item.SubType === 'Managed Counter') {
		status = '' + this.item.Counter;
	} else {
		status = '' + this.translationService.t('Usage') + ': ' + this.item.CounterToday;
	}
      } else {
        if ((typeof this.item.CounterDeliv !== 'undefined') && (this.item.CounterDeliv !== '0')) {
          status = 'U: T: ' + this.item.CounterToday;
        } else {
		if (this.item.SubType === 'Managed Counter') {
			status = '' + this.item.Counter;
		} else {
			status = 'T: ' + this.item.CounterToday;
		}
        }
      }
    } else if (this.item.Type === 'Current') {
      status = this.item.Data;
    } else if (
      (this.item.Type === 'Energy') ||
      (this.item.Type === 'Current/Energy') ||
      (this.item.Type === 'Power') ||
      (this.item.SubType === 'kWh') ||
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
      (this.item.SubType === 'Waterflow') ||
      (this.item.SubType === 'Sound Level') ||
      (this.item.SubType === 'Custom Sensor')
    ) {
      if (typeof this.item.CounterToday !== 'undefined') {
        status += 'T: ' + this.item.CounterToday;
      } else {
        status = this.item.Data;
      }
    } else if (this.item.SubType === 'Alert') {
      let aLevel = this.item.Level;
      if (aLevel > 4) {
        aLevel = 4;
      }
      status = this.item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2') +
        ' <img src="images/Alert48_' + aLevel + '.png" height="16" width="16">';
    } else if (this.item.SubType === 'Text') {
      status = this.item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
    } else if (this.item.SubType === 'Smartwares') {
      status = this.item.Data + '\u00B0 ' + this.configService.config.TempSign;
    } else if ((this.item.SubType === 'Thermostat Mode') || (this.item.SubType === 'Thermostat Fan Mode')) {
      status = this.item.Data;
    }

    return status;
  }

  ShowSetpointPopup(event: any) {
    this.setpointPopup.ShowSetpointPopup(event, this.item.idx, this.item.Protected, this.item.Data, true);
  }

  onSetpointSet() {
    this.deviceService.getDeviceInfo(this.item.idx).subscribe(device => {
      this.item = device;
    });
  }

  get buttonText(): string {
    if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      return this.item.Data + '\u00B0 ' + this.configService.config.TempSign;
    } else if (this.item.SubType === 'Smartwares') {
      return this.translationService.t('Set');
    }
  }

  get status2(): string {
    let status = '';

    let bHaveReturnUsage = false;
    if (typeof this.item.CounterDeliv !== 'undefined') {
      if (this.item.UsageDeliv.charAt(0) !== '0') {
        bHaveReturnUsage = true;
      }
    }

    if (typeof this.item.Usage !== 'undefined') {
      if (this.configService.config.DashboardType === 0) {
        status += '<br>' + this.translationService.t('Actual') + ': ' + this.item.Usage;
      } else {
        if (!bHaveReturnUsage) {
          status += ', A: ' + this.item.Usage;
        }
      }
    }

    if (typeof this.item.CounterDeliv !== 'undefined') {
      if (this.item.CounterDeliv !== '0') {
        if (this.configService.config.DashboardType === 0) {
          status += '<br>' + this.translationService.t('Return') + ': ' + this.item.CounterDelivToday;
          status += '<br>' + this.translationService.t('Actual') + ': -' + this.item.UsageDeliv;
        } else {
          status += '<br>R: T: ' + this.item.CounterDelivToday;
          if (bHaveReturnUsage) {
            status += ', A: ';
            if (Number(this.item.UsageDeliv) > 0) {
              status += '-';
            }
            status += this.item.UsageDeliv;
          }
        }
      }
    }

    return status;
  }

}
