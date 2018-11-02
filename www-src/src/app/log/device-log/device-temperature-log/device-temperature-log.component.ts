import { Component, OnInit, OnDestroy, Input } from '@angular/core';
import {Device} from '../../../_shared/_models/device';

@Component({
  selector: 'dz-device-temperature-log',
  templateUrl: './device-temperature-log.component.html',
  styleUrls: ['./device-temperature-log.component.css']
})
export class DeviceTemperatureLogComponent implements OnInit, OnDestroy {

  @Input()
  device: Device;

  constructor() { }

  ngOnInit() {
  }

  ngOnDestroy(): void {
  }

}
