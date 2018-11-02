import { Component, OnInit, Inject } from '@angular/core';
import { Hardware } from 'src/app/_shared/_models/hardware';
import { ActivatedRoute, Router } from '@angular/router';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';

@Component({
  selector: 'dz-edit-rego6xx-type',
  templateUrl: './edit-rego6xx-type.component.html',
  styleUrls: ['./edit-rego6xx-type.component.css']
})
export class EditRego6xxTypeComponent implements OnInit {

  hardware: Hardware;

  comborego6xxtype: string;

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

      this.comborego6xxtype = hardware.Mode1.toString();
    });
  }

  SetRego6XXType() {
    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    formData.append('Rego6XXType', this.comborego6xxtype);
    this.hardwareService.setRego6XXType(formData).subscribe(() => {
      this.router.navigate(['/Hardware']);
    });
  }

}
