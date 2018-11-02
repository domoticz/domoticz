import { Component, OnInit, Inject } from '@angular/core';
import { Hardware } from 'src/app/_shared/_models/hardware';
import { ActivatedRoute, Router } from '@angular/router';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';

@Component({
  selector: 'dz-hardware-edit-ccusb',
  templateUrl: './hardware-edit-ccusb.component.html',
  styleUrls: ['./hardware-edit-ccusb.component.css']
})
export class HardwareEditCcusbComponent implements OnInit {

  hardware: Hardware;

  CCBaudrate: string;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private router: Router,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;

      this.CCBaudrate = hardware.Address;
    });
  }

  SetCCUSBType() {
    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    formData.append('CCBaudrate', this.CCBaudrate);
    this.hardwareService.setCCUSBType(formData).subscribe(() => {
      this.router.navigate(['/Hardware']);
    });
  }

}
