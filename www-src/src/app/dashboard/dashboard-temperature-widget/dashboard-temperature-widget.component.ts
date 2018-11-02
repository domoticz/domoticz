import { Component, OnInit, Input, Inject } from '@angular/core';
import { ConfigService } from '../../_shared/_services/config.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { Utils } from 'src/app/_shared/_utils/utils';
import { DeviceService } from 'src/app/_shared/_services/device.service';
import {Device} from "../../_shared/_models/device";

@Component({
  selector: 'dz-dashboard-temperature-widget',
  templateUrl: './dashboard-temperature-widget.component.html',
  styleUrls: ['./dashboard-temperature-widget.component.css']
})
export class DashboardTemperatureWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
  }

  get backgroundClass(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  get bigtext(): string {
    let bigtext = '';
    if (typeof this.item.Temp !== 'undefined') {
      bigtext = this.item.Temp + '\u00B0 ' + this.configService.config.TempSign;
    }
    if (typeof this.item.Humidity !== 'undefined') {
      if (bigtext !== '') {
        bigtext += ' / ';
      }
      bigtext += this.item.Humidity + '%';
    }
    if (typeof this.item.Chill !== 'undefined') {
      if (bigtext !== '') {
        bigtext += ' / ';
      }
      bigtext += this.item.Chill + '\u00B0 ' + this.configService.config.TempSign;
    }
    return bigtext;
  }

  get imageSrc(): string {
    if (typeof this.item.Temp !== 'undefined') {
      return this.deviceService.GetTemp48Item(this.item.Temp);
    } else {
      if (this.item.Type === 'Humidity') {
        return 'gauge48.png';
      } else {
        return this.deviceService.GetTemp48Item(this.item.Chill);
      }
    }
  }

  get status(): string {
    let bHaveBefore = false;
    let status = '';
    if (typeof this.item.HumidityStatus !== 'undefined') {
      status += this.translationService.t(this.item.HumidityStatus);
      bHaveBefore = true;
    }
    if (typeof this.item.DewPoint !== 'undefined') {
      if (bHaveBefore === true) {
        status += ', ';
      }
      status += this.translationService.t('Dew Point') + ': ' + this.item.DewPoint + '° ' + this.configService.config.TempSign;
    }
    return status;
  }

}
