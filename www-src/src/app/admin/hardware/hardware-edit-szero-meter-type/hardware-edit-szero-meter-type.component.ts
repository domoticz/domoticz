import {Component, OnInit} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {HardwareService} from 'src/app/_shared/_services/hardware.service';
import {Hardware} from 'src/app/_shared/_models/hardware';

@Component({
  selector: 'dz-hardware-edit-szero-meter-type',
  templateUrl: './hardware-edit-szero-meter-type.component.html',
  styleUrls: ['./hardware-edit-szero-meter-type.component.css']
})
export class HardwareEditSzeroMeterTypeComponent implements OnInit {

  hardware: Hardware;

  combom1type: string;
  M1PulsesPerHour: string;
  combom2type: string;
  M2PulsesPerHour: string;
  combom3type: string;
  M3PulsesPerHour: string;
  combom4type: string;
  M4PulsesPerHour: string;
  combom5type: string;
  M5PulsesPerHour: string;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private router: Router
  ) {
  }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;

      const res = hardware.Extra.split(';');
      this.combom1type = res[0];
      if (res[1] !== '0') {
        this.M1PulsesPerHour = res[1];
      } else {
        this.M1PulsesPerHour = '2000';
      }
      this.combom2type = res[2];
      if (res[3] !== '0') {
        this.M2PulsesPerHour = res[3];
      } else {
        this.M2PulsesPerHour = '2000';
      }
      this.combom3type = res[4];
      if (res[5] !== '0') {
        this.M3PulsesPerHour = res[5];
      } else {
        this.M3PulsesPerHour = '2000';
      }
      this.combom4type = res[6];
      if (res[7] !== '0') {
        this.M4PulsesPerHour = res[7];
      } else {
        this.M4PulsesPerHour = '2000';
      }
      this.combom5type = res[8];
      if (res[9] !== '0') {
        this.M5PulsesPerHour = res[9];
      } else {
        this.M5PulsesPerHour = '2000';
      }
    });
  }

  SetS0MeterType() {
    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    formData.append('combom1type', this.combom1type);
    formData.append('M1PulsesPerHour', this.M1PulsesPerHour);
    formData.append('combom2type', this.combom2type);
    formData.append('M2PulsesPerHour', this.M2PulsesPerHour);
    formData.append('combom3type', this.combom3type);
    formData.append('M3PulsesPerHour', this.M3PulsesPerHour);
    formData.append('combom4type', this.combom4type);
    formData.append('M4PulsesPerHour', this.M4PulsesPerHour);
    formData.append('combom5type', this.combom5type);
    formData.append('M5PulsesPerHour', this.M5PulsesPerHour);
    this.hardwareService.setS0MeterType(formData).subscribe(() => {
      this.router.navigate(['/Hardware']);
    });
  }

}
