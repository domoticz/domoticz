import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { Hardware } from 'src/app/_shared/_models/hardware';

@Component({
  selector: 'dz-hardware-edit-limitless-type',
  templateUrl: './hardware-edit-limitless-type.component.html',
  styleUrls: ['./hardware-edit-limitless-type.component.css']
})
export class HardwareEditLimitlessTypeComponent implements OnInit {

  hardware: Hardware;

  combom1type: string;
  CCBridgeType: string;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private router: Router
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;

      this.combom1type = hardware.Mode1.toString();
      this.CCBridgeType = hardware.Mode2.toString();
    });
  }

  SetLimitlessType() {
    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    formData.append('combom1type', this.combom1type);
    formData.append('CCBridgeType', this.CCBridgeType);
    this.hardwareService.setLimitlessType(formData).subscribe(() => {
      this.router.navigate(['/Hardware']);
    });
  }

}
