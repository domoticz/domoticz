import {Component, Input, OnInit} from '@angular/core';
import {Device} from '../../../_shared/_models/device';

@Component({
  selector: 'dz-device-graph-log',
  templateUrl: './device-graph-log.component.html',
  styleUrls: ['./device-graph-log.component.css']
})
export class DeviceGraphLogComponent implements OnInit {

  @Input() device: Device;

  constructor() {
  }

  ngOnInit() {
  }

}
