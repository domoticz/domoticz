import { Component, OnInit, Inject } from '@angular/core';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';

@Component({
  selector: 'dz-create-rf-link-device-dialog',
  templateUrl: './create-rf-link-device-dialog.component.html',
  styleUrls: ['./create-rf-link-device-dialog.component.css']
})
export class CreateRfLinkDeviceDialogComponent extends DialogComponent implements OnInit {

  private idx: string;

  sensorname: string;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    @Inject(DIALOG_DATA) data: any,
    private notificationService: NotificationService,
    private hardwareService: HardwareService) {

    super(dialogService);

    this.idx = data.idx;
  }

  ngOnInit() {
  }

  protected getDialogOptions(): any {
    return {
      width: 420,
      height: 250
    };
  }

  protected getDialogId(): string {
    return 'dialog-createrflinkdevice';
  }

  protected getDialogTitle(): string {
    return 'Manual Create RFLink devices';
  }

  protected getDialogButtons() {
    return {
      'OK': () => {
        const bValid = true;
        this.close();
        const SensorName = this.sensorname;
        if (SensorName === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter a command!'), 2500, true);
          return;
        }
        this.hardwareService.createRfLinkDevice(this.idx, SensorName).subscribe((data) => {
          if (data.status === 'OK') {
            this.notificationService.ShowNotify(this.translationService.t('Device created, it can be found in the devices tab!'), 2500);
          } else {
            this.notificationService.ShowNotify(this.translationService.t('Problem creating Sensor!'), 2500, true);
          }
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem creating Sensor!'), 2500, true);
        });
      },
      Cancel: () => {
        this.close();
      }
    };
  }

}
