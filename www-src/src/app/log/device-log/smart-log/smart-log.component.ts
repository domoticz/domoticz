import { Component, OnInit, Input } from '@angular/core';
import { DeviceService } from '../../../_shared/_services/device.service';
import { Costs, CostsResponse } from 'src/app/_shared/_models/costs';
import {Device} from "../../../_shared/_models/device";

@Component({
  selector: 'dz-smart-log',
  templateUrl: './smart-log.component.html',
  styleUrls: ['./smart-log.component.css']
})
export class SmartLogComponent implements OnInit {

  @Input() device: Device;

  costs: Costs = {
    CostEnergy: 0.2389,
    CostEnergyT2: 0.2389,
    CostEnergyR1: 0.08,
    CostEnergyR2: 0.08,
    CostGas: 0.6218,
    CostWater: 1.6473
  };

  constructor(
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
    // FIXME does the costs serves to something here ?! I don't believe so... !
    this.deviceService.getCosts(this.device.idx).subscribe((costs: CostsResponse) => {
      this.costs = costs;
    });
  }

}
