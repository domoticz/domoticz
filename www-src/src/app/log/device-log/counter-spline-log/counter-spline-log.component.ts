import { Component, OnInit, Input } from '@angular/core';
import {Device} from "../../../_shared/_models/device";

@Component({
  selector: 'dz-counter-spline-log',
  templateUrl: './counter-spline-log.component.html',
  styleUrls: ['./counter-spline-log.component.css']
})
export class CounterSplineLogComponent implements OnInit {

  @Input() device: Device;

  constructor() { }

  ngOnInit() {
  }

}
