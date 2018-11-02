import {Component, OnInit} from '@angular/core';
import {ConfigService} from '../../_shared/_services/config.service';
import {DomSanitizer, SafeResourceUrl} from '@angular/platform-browser';
import {LocationService} from '../location.service';

@Component({
  selector: 'dz-forecast',
  templateUrl: './forecast.component.html',
  styleUrls: ['./forecast.component.css']
})
export class ForecastComponent implements OnInit {

  iframeSrc: SafeResourceUrl;

  constructor(
    private configService: ConfigService,
    private locationService: LocationService,
    private sanitizer: DomSanitizer) {
  }

  ngOnInit() {
    this.locationService.getLocation().subscribe(data => {
      if (typeof data.Latitude !== 'undefined') {
        let units = 'ca24';
        if (this.configService.config.TempSign === 'F') {
          units = 'us12';
        } else {
          if (this.configService.config.WindSign === 'm/s') {
            units = 'si24';
          } else if (this.configService.config.WindSign === 'km/h') {
            units = 'ca24';
          } else if (this.configService.config.WindSign === 'mph') {
            units = 'uk224';
          }
        }

        // const src = '//darksky.net/forecast/' + data.Latitude + ',' + data.Longitude + '/' +
        //   units + '/' + this.configService.config.language;
        const src = '//forecast.io/embed/#lat=' + data.Latitude + '&lon=' + data.Longitude + '&units=ca&color=#00aaff';
        this.iframeSrc = this.sanitizer.bypassSecurityTrustResourceUrl(src);
      }
    });
  }

}
