import {Component, OnInit} from '@angular/core';
import {DomSanitizer, SafeResourceUrl} from '@angular/platform-browser';
import {DeviceService} from '../../_shared/_services/device.service';
import {ActivatedRoute} from '@angular/router';

@Component({
  selector: 'dz-weather-forecast',
  templateUrl: './weather-forecast.component.html',
  styleUrls: ['./weather-forecast.component.css']
})
export class WeatherForecastComponent implements OnInit {

  forecast_url: SafeResourceUrl;

  constructor(private deviceService: DeviceService,
              private route: ActivatedRoute,
              private sanitizer: DomSanitizer) {
  }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.deviceService.getDeviceInfo(idx).subscribe(device => {
      this.forecast_url = this.sanitizer.bypassSecurityTrustUrl(device.forecast_url);
    });
  }

}
