import { Component, OnInit, Input, Inject } from '@angular/core';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import {Device} from "../../_shared/_models/device";

@Component({
  selector: '[dzDashboardMobileTemperatureWidget]',
  templateUrl: './dashboard-mobile-temperature-widget.component.html'
})
export class DashboardMobileTemperatureWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) { }

  ngOnInit() {
  }

  get status(): string {
    let status = '';
    let bHaveBefore = false;
    if (typeof this.item.Temp !== 'undefined') {
      status += this.item.Temp + '° ' + this.configService.config.TempSign;
      bHaveBefore = true;
    }
    if (typeof this.item.Chill !== 'undefined') {
      if (bHaveBefore) {
        status += ', ';
      }
      status += this.translationService.t('Chill') + ': ' + this.item.Chill + '° ' + this.configService.config.TempSign;
      bHaveBefore = true;
    }
    if (typeof this.item.Humidity !== 'undefined') {
      if (bHaveBefore === true) {
        status += ', ';
      }
      status += this.item.Humidity + ' %';
    }
    if (typeof this.item.HumidityStatus !== 'undefined') {
      status += ' (' + this.translationService.t(this.item.HumidityStatus) + ')';
    }
    return status;
  }

  get status2ndline(): string | undefined {
    if (typeof this.item.DewPoint !== 'undefined') {
      return this.translationService.t('Dew Point') + ': ' + this.item.DewPoint + '° ' + this.configService.config.TempSign;
    } else {
      return undefined;
    }
  }

}
