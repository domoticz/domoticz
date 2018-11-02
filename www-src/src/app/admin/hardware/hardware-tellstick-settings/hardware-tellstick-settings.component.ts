import { Component, OnInit, Inject } from '@angular/core';
import { Hardware } from 'src/app/_shared/_models/hardware';
import { ActivatedRoute, Router } from '@angular/router';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { NotificationService } from '../../../_shared/_services/notification.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-hardware-tellstick-settings',
  templateUrl: './hardware-tellstick-settings.component.html',
  styleUrls: ['./hardware-tellstick-settings.component.css']
})
export class HardwareTellstickSettingsComponent implements OnInit {

  hardware: Hardware;

  repeats: string;
  repeatInterval: string;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private router: Router,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;

      this.repeats = hardware.Mode1.toString();
      this.repeatInterval = hardware.Mode2.toString();
    });
  }

  TellstickApplySettings() {
    let repeats = Number(this.repeats);
    if (repeats < 0) {
      repeats = 0;
    }
    if (repeats > 10) {
      repeats = 10;
    }
    let repeatInterval = Number(this.repeatInterval);
    if (repeatInterval < 10) {
      repeatInterval = 10;
    }
    if (repeatInterval > 2000) {
      repeatInterval = 2000;
    }

    this.hardwareService.tellstickApplySettings(this.hardware.idx.toString(), repeats, repeatInterval).subscribe(() => {
      bootbox.alert(this.translationService.t('Settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Failed saving settings'), 2500, true);
    });
  }

}
