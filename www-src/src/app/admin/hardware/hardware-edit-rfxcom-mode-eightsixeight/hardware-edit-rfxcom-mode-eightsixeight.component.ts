import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { ActivatedRoute, Router } from '@angular/router';
import { Component, OnInit, Inject } from '@angular/core';
import { Hardware } from 'src/app/_shared/_models/hardware';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';

@Component({
  selector: 'dz-hardware-edit-rfxcom-mode-eightsixeight',
  templateUrl: './hardware-edit-rfxcom-mode-eightsixeight.component.html',
  styleUrls: ['./hardware-edit-rfxcom-mode-eightsixeight.component.css']
})
export class HardwareEditRfxcomModeEightsixeightComponent implements OnInit {

  hardware: Hardware;

  LaCrosse: boolean;
  Alecto: boolean;
  Legrand: boolean;
  ProGuard: boolean;
  VionicPowerCode: boolean;
  Hideki: boolean;
  FHT8: boolean;
  VionicCodeSecure: boolean;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private router: Router
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;
    });
  }

  Default() {
    this.LaCrosse = false;
    this.Alecto = false;
    this.Legrand = false;
    this.ProGuard = false;
    this.VionicPowerCode = false;
    this.Hideki = false;
    this.FHT8 = false;
    this.VionicCodeSecure = false;
  }

  SetRFXCOMMode868() {
    this.notificationService.HideNotify();
    this.notificationService.ShowNotify(this.translationService.t('This should (for now) be set via the RFXmngr application!'), 2500, true);
    /*
    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    if (this.LaCrosse) {
      formData.append('LaCrosse', 'on');
    }
    if (this.Alecto) {
      formData.append('Alecto', 'on');
    }
    if (this.Legrand) {
      formData.append('Legrand', 'on');
    }
    if (this.ProGuard) {
      formData.append('ProGuard', 'on');
    }
    if (this.VionicPowerCode) {
      formData.append('VionicPowerCode', 'on');
    }
    if (this.Hideki) {
      formData.append('Hideki', 'on');
    }
    if (this.FHT8) {
      formData.append('FHT8', 'on');
    }
    if (this.VionicCodeSecure) {
      formData.append('VionicCodeSecure', 'on');
    }
    this.hardwareService.setRFXCOMMode(formData).subscribe(() => {
      this.router.navigate(['/dashboard']);
    });
    */
  }

}
