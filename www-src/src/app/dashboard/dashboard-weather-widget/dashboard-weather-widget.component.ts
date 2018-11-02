import {Component, Inject, Input, OnInit} from '@angular/core';
import {ConfigService} from 'src/app/_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Utils} from 'src/app/_shared/_utils/utils';
import {Device} from '../../_shared/_models/device';

@Component({
  selector: 'dz-dashboard-weather-widget',
  templateUrl: './dashboard-weather-widget.component.html',
  styleUrls: ['./dashboard-weather-widget.component.css']
})
export class DashboardWeatherWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  ngOnInit() {
  }

  get backgroundClass(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  get bigtext(): string {
    let bigtext = '';
    if (typeof this.item.Barometer !== 'undefined') {
      bigtext += this.item.Barometer + ' hPa';
    } else if (typeof this.item.Rain !== 'undefined') {
      bigtext += this.item.Rain + ' mm';
    } else if (typeof this.item.Visibility !== 'undefined') {
      bigtext += this.item.Data;
    } else if (typeof this.item.UVI !== 'undefined') {
      bigtext += this.item.UVI + ' UVI';
    } else if (typeof this.item.Radiation !== 'undefined') {
      bigtext += this.item.Data;
    } else if (typeof this.item.Direction !== 'undefined') {
      bigtext += this.item.DirectionStr;
      if (typeof this.item.Speed !== 'undefined') {
        bigtext += ' / ' + this.item.Speed + ' ' + this.configService.config.WindSign;
      } else if (typeof this.item.Gust !== 'undefined') {
        bigtext += ' / ' + this.item.Gust + ' ' + this.configService.config.WindSign;
      }
    }
    return bigtext;
  }

  get imageLink(): string[] {
    if (typeof this.item.Rain !== 'undefined') {
      return ['/RainLog', this.item.idx];
    } else if (typeof this.item.Visibility !== 'undefined') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (typeof this.item.UVI !== 'undefined') {
      return ['/UVLog', this.item.idx];
    } else if (typeof this.item.Radiation !== 'undefined') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (typeof this.item.Direction !== 'undefined') {
      return ['/WindLog', this.item.idx];
    } else if (typeof this.item.Barometer !== 'undefined') {
      return ['/BaroLog', this.item.idx];
    }
  }

  get imageSrc(): string {
    if (typeof this.item.Rain !== 'undefined') {
      return 'Rain48_On.png';
    } else if (typeof this.item.Visibility !== 'undefined') {
      return 'visibility48.png';
    } else if (typeof this.item.UVI !== 'undefined') {
      return 'uv48.png';
    } else if (typeof this.item.Radiation !== 'undefined') {
      return 'radiation48.png';
    } else if (typeof this.item.Direction !== 'undefined') {
      return 'Wind' + this.item.DirectionStr + '.png';
    } else if (typeof this.item.Barometer !== 'undefined') {
      return 'baro48.png';
    }
  }

  get status(): string {
    let status = '';
    if (typeof this.item.Rain !== 'undefined') {
      if (typeof this.item.RainRate !== 'undefined') {
        status += 'Rate: ' + this.item.RainRate + ' mm/h';
      }
    } else if (typeof this.item.Visibility !== 'undefined') {
    } else if (typeof this.item.UVI !== 'undefined') {
    } else if (typeof this.item.Radiation !== 'undefined') {
    } else if (typeof this.item.Direction !== 'undefined') {
      status += this.item.Direction + ' ' + this.item.DirectionStr;
      if (typeof this.item.Speed !== 'undefined') {
        status += ', ' + this.translationService.t('Speed') + ': ' + this.item.Speed + ' ' + this.configService.config.WindSign;
      }
      if (typeof this.item.Gust !== 'undefined') {
        status += ', ' + this.translationService.t('Gust') + ': ' + this.item.Gust + ' ' + this.configService.config.WindSign;
      }
      status += '<br>\n';
      if (typeof this.item.Temp !== 'undefined') {
        status += this.translationService.t('Temp') + ': ' + this.item.Temp + '° ' + this.configService.config.TempSign;
      }
      if (typeof this.item.Chill !== 'undefined') {
        if (typeof this.item.Temp !== 'undefined') {
          status += ', ';
        }
        status += this.translationService.t('Chill') + ': ' + this.item.Chill + '° ' + this.configService.config.TempSign;
      }
    } else if (typeof this.item.Barometer !== 'undefined') {
      if (typeof this.item.ForecastStr !== 'undefined') {
        status += this.translationService.t('Prediction') + ': ' + this.translationService.t(this.item.ForecastStr);
      }
      if (typeof this.item.Altitude !== 'undefined') {
        status += ', Altitude: ' + this.item.Altitude + ' meter';
      }
    }
    return status;
  }

}
