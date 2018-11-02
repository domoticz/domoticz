import {Component, OnInit} from '@angular/core';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {ActivatedRoute, Router} from '@angular/router';
import {HardwareService} from 'src/app/_shared/_services/hardware.service';
import {DeviceService} from '../../../_shared/_services/device.service';
import {Device} from '../../../_shared/_models/device';

@Component({
  selector: 'dz-hardware-edit-open-term',
  templateUrl: './hardware-edit-open-term.component.html',
  styleUrls: ['./hardware-edit-open-term.component.css']
})
export class HardwareEditOpenTermComponent implements OnInit {

  hardware: Hardware;
  devices: Array<Device> = [];

  combooutsidesensor: string;
  otgwcmnd: string;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private deviceService: DeviceService,
    private router: Router
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;

      // Mode1=Outside Temperature Sensor DeviceIdx, 0=Not Using
      this.combooutsidesensor = hardware.Mode1.toString();
    });
    this.deviceService.getDevices('temp', true, 'Name').subscribe(devices => {
      this.devices = devices.result.filter(_ => typeof _.Temp !== 'undefined');
    });
  }

  SetOpenThermSettings() {
    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    formData.append('combooutsidesensor', this.combooutsidesensor);
    formData.append('otgwcmnd', this.otgwcmnd);
    this.hardwareService.setOpenThermSettings(formData).subscribe(() => {
      this.router.navigate(['/Hardware']);
    });
  }

  SendOTGWCommand() {
    const idx = this.hardware.idx.toString();
    const cmnd = this.otgwcmnd;
    this.hardwareService.sendOpenThermCommand(idx, cmnd).subscribe(() => {
      // Noop
    }, error => {
      // Noop
    });
  }

}
