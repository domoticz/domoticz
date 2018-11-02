import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { HardwareService } from '../../../_shared/_services/hardware.service';
import { Hardware } from 'src/app/_shared/_models/hardware';

@Component({
  selector: 'dz-hardware-setup',
  templateUrl: './hardware-setup.component.html',
  styleUrls: ['./hardware-setup.component.css']
})
export class HardwareSetupComponent implements OnInit {

  hardware: Hardware;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');

    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;
    });
  }

}
