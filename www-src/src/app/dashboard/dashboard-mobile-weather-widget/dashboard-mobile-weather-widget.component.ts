import { Component, OnInit, Input, Inject } from '@angular/core';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import {Device} from "../../_shared/_models/device";

@Component({
  selector: '[dzDashboardMobileWeatherWidget]',
  templateUrl: './dashboard-mobile-weather-widget.component.html',
  styleUrls: ['./dashboard-mobile-weather-widget.component.css']
})
export class DashboardMobileWeatherWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) { }

  ngOnInit() {
  }

  get nextLink(): string[] {
    if (typeof this.item.UVI !== 'undefined') {
      return ['/UVLog', this.item.idx];
    } else if (typeof this.item.Visibility !== 'undefined' || typeof this.item.Radiation !== 'undefined') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (typeof this.item.Direction !== 'undefined') {
      return ['/WindLog', this.item.idx];
    } else if (typeof this.item.Rain !== 'undefined') {
      return ['/RainLog', this.item.idx];
    } else if (typeof this.item.Barometer !== 'undefined') {
      return ['/BaroLog', this.item.idx];
    }
  }

  get status(): string {
    let status = '';
    if (typeof this.item.Rain !== 'undefined') {
      status += this.item.Rain + ' mm';
      if (typeof this.item.RainRate !== 'undefined') {
        status += ', Rate: ' + this.item.RainRate + ' mm/h';
      }
    } else if (typeof this.item.Visibility !== 'undefined') {
      status += this.item.Data;
    } else if (typeof this.item.UVI !== 'undefined') {
      status += this.item.UVI + ' UVI';
    } else if (typeof this.item.Radiation !== 'undefined') {
      status += this.item.Data;
    } else if (typeof this.item.Direction !== 'undefined') {
      status = this.item.Direction + ' ' + this.item.DirectionStr;
      if (typeof this.item.Speed !== 'undefined') {
        status += ', ' + this.translationService.t('Speed') + ': ' + this.item.Speed + ' ' + this.configService.config.WindSign;
      }
      if (typeof this.item.Gust !== 'undefined') {
        status += ', ' + this.translationService.t('Gust') + ': ' + this.item.Gust + ' ' + this.configService.config.WindSign;
      }
    } else if (typeof this.item.Barometer !== 'undefined') {
      if (typeof this.item.ForecastStr !== 'undefined') {
        status = this.item.Barometer + ' hPa, ' + this.translationService.t('Prediction') + ': ' +
          this.translationService.t(this.item.ForecastStr);
      } else {
        status = this.item.Barometer + ' hPa';
      }
      if (typeof this.item.Altitude !== 'undefined') {
        status += ', Altitude: ' + this.item.Altitude + ' meter';
      }
    }
    return status;
  }

}
