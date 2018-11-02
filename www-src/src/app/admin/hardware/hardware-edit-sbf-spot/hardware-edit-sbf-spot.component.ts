import { Component, OnInit, Inject } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { Hardware } from 'src/app/_shared/_models/hardware';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-hardware-edit-sbf-spot',
  templateUrl: './hardware-edit-sbf-spot.component.html',
  styleUrls: ['./hardware-edit-sbf-spot.component.css']
})
export class HardwareEditSbfSpotComponent implements OnInit {

  hardware: Hardware;

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
    });
  }

  ImportOldData() {
    bootbox.alert(
      this.translationService.t('Importing old data, this could take a while!<br>You should automaticly return on the Dashboard'));

    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    this.hardwareService.importOldDataSbfSpot(formData).subscribe(() => {
      this.router.navigate(['/dashboard']);
    });
  }

}
