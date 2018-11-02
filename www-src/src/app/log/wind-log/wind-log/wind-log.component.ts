import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { DeviceService } from 'src/app/_shared/_services/device.service';
import { ConfigService } from '../../../_shared/_services/config.service';
import {Device} from "../../../_shared/_models/device";

@Component({
  selector: 'dz-wind-log',
  templateUrl: './wind-log.component.html',
  styleUrls: ['./wind-log.component.css']
})
export class WindLogComponent implements OnInit {

  idx: string;
  device: Device;

  lscales: Array<{ from: number, to: number }> = [];

  constructor(
    private route: ActivatedRoute,
    private deviceService: DeviceService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
    this.idx = this.route.snapshot.paramMap.get('idx');

    // $('#modal').show();

    this.deviceService.getDeviceInfo(this.idx).subscribe(device => {
      this.device = device;
    });

    if (this.configService.config.WindSign === 'bf') {
      this.lscales.push({ from: 0, to: 1 });
      this.lscales.push({ from: 1, to: 2 });
      this.lscales.push({ from: 2, to: 3 });
      this.lscales.push({ from: 3, to: 4 });
      this.lscales.push({ from: 4, to: 5 });
      this.lscales.push({ from: 5, to: 6 });
      this.lscales.push({ from: 6, to: 7 });
      this.lscales.push({ from: 7, to: 8 });
      this.lscales.push({ from: 8, to: 9 });
      this.lscales.push({ from: 9, to: 10 });
      this.lscales.push({ from: 10, to: 11 });
      this.lscales.push({ from: 11, to: 12 });
      this.lscales.push({ from: 12, to: 100 });
    } else {
      this.lscales.push({ from: 0.3 * this.configService.config.WindScale, to: 1.5 * this.configService.config.WindScale });
      this.lscales.push({ from: 1.5 * this.configService.config.WindScale, to: 3.3 * this.configService.config.WindScale });
      this.lscales.push({ from: 3.3 * this.configService.config.WindScale, to: 5.5 * this.configService.config.WindScale });
      this.lscales.push({ from: 5.5 * this.configService.config.WindScale, to: 8 * this.configService.config.WindScale });
      this.lscales.push({ from: 8.0 * this.configService.config.WindScale, to: 10.8 * this.configService.config.WindScale });
      this.lscales.push({ from: 10.8 * this.configService.config.WindScale, to: 13.9 * this.configService.config.WindScale });
      this.lscales.push({ from: 13.9 * this.configService.config.WindScale, to: 17.2 * this.configService.config.WindScale });
      this.lscales.push({ from: 17.2 * this.configService.config.WindScale, to: 20.8 * this.configService.config.WindScale });
      this.lscales.push({ from: 20.8 * this.configService.config.WindScale, to: 24.5 * this.configService.config.WindScale });
      this.lscales.push({ from: 24.5 * this.configService.config.WindScale, to: 28.5 * this.configService.config.WindScale });
      this.lscales.push({ from: 28.5 * this.configService.config.WindScale, to: 32.7 * this.configService.config.WindScale });
      this.lscales.push({ from: 32.7 * this.configService.config.WindScale, to: 100 * this.configService.config.WindScale });
      this.lscales.push({ from: 32.7 * this.configService.config.WindScale, to: 100 * this.configService.config.WindScale });
    }
  }

}
