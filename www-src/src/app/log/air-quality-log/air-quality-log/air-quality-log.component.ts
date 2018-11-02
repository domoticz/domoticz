import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { DeviceService } from 'src/app/_shared/_services/device.service';
import {Device} from "../../../_shared/_models/device";

@Component({
  selector: 'dz-air-quality-log',
  templateUrl: './air-quality-log.component.html',
  styleUrls: ['./air-quality-log.component.css']
})
export class AirQualityLogComponent implements OnInit {

  idx: string;
  device: Device;

  constructor(
    private route: ActivatedRoute,
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
    this.idx = this.route.snapshot.paramMap.get('idx');

    // $('#modal').show();

    this.deviceService.getDeviceInfo(this.idx).subscribe(device => {
      this.device = device;
    });
  }

}
