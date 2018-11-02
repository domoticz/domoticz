import { HttpErrorResponse } from '@angular/common/http';
import { Component, OnInit, OnDestroy, Inject } from '@angular/core';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { ActivatedRoute, Router } from '@angular/router';
import { interval, Subscription } from 'rxjs';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-rfxcom-firmware',
  templateUrl: './rfxcom-firmware.component.html',
  styleUrls: ['./rfxcom-firmware.component.css']
})
export class RfxcomFirmwareComponent implements OnInit, OnDestroy {

  idx: string;
  file: string | ArrayBuffer;
  selected_file = '';
  isUpdating = false;
  topText = 'Updating Firmware...';
  bottomText = '';
  progressDataPercentage = 0;

  mytimer: Subscription;

  constructor(
    private hardwareService: HardwareService,
    private route: ActivatedRoute,
    private router: Router,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) { }

  ngOnInit() {
    this.idx = this.route.snapshot.paramMap.get('idx');
    this.bottomText = '';
    this.isUpdating = false;

    this.SetPercentage(0);
  }

  ngOnDestroy() {
    if (this.mytimer) {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
  }

  SetPercentage(percentage: number) {
    const perc = Number(percentage.toFixed(2));
    this.progressDataPercentage = perc;
  }

  onFileChange(event: any) {
    const reader = new FileReader();
    if (event.target.files && event.target.files.length > 0) {
      const file = event.target.files[0];
      reader.readAsDataURL(file);
      reader.onload = () => {
        this.file = reader.result;
      };
    }
  }

  UpdateFirmware() {
    if (typeof this.file === 'undefined') {
      return;
    }
    this.hardwareService.upgradeFirmware(this.idx, this.file as any)
      .subscribe(() => {
        this.isUpdating = true;
        this.mytimer = interval(1000).subscribe(() => {
          this.progressupload();
        });
      }, error => {
        this.router.navigate(['/dashboard']);
      });
  }

  progressupload() {
    if (this.mytimer) {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    this.hardwareService.getRfxFirmwareUpdatePercentage(this.idx)
      .subscribe(data => {
        if (data.status === 'ERR') {
          this.notificationService.HideNotify();
          bootbox.alert(this.translationService.t('Problem updating firmware') + '<br>' + this.translationService.t(data.message));
          this.router.navigate(['/dashboard']);
          return;
        }
        const percentage = Number(data.percentage);
        if (percentage === -1) {
          this.notificationService.HideNotify();
          bootbox.alert(this.translationService.t('Problem updating firmware') + '<br>' + this.translationService.t(data.message));
          this.router.navigate(['/dashboard']);
        } else if (percentage === 100) {
          this.router.navigate(['/dashboard']);
        } else {
          this.progressDataPercentage = Math.round(data.percentage);
          this.bottomText = data.message;
          this.mytimer = interval(1000).subscribe(() => {
            this.progressupload();
          });
        }
      }, (error: HttpErrorResponse) => {
        this.notificationService.HideNotify();
        bootbox.alert(this.translationService.t('Problem updating firmware') + '<br>' + this.translationService.t(error.message));
        this.router.navigate(['/dashboard']);
      });
  }

}
