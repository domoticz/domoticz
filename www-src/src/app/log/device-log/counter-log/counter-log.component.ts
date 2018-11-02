import { Component, OnInit, Input } from '@angular/core';
import {Device} from "../../../_shared/_models/device";

@Component({
  selector: 'dz-counter-log',
  templateUrl: './counter-log.component.html',
  styleUrls: ['./counter-log.component.css']
})
export class CounterLogComponent implements OnInit {

  @Input() device: Device;

  constructor() { }

  ngOnInit() {
  }

}
