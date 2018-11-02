import { Component, OnInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-create-evohome-sensors-dialog',
  templateUrl: './create-evohome-sensors-dialog.component.html',
  styleUrls: ['./create-evohome-sensors-dialog.component.css']
})
export class CreateEvohomeSensorsDialogComponent extends DialogComponent implements OnInit {

  private idx: string;

  sensortype: string;

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
      width: 380,
      height: 160
    };
  }

  protected getDialogId(): string {
    return 'dialog-createevohomesensor';
  }

  protected getDialogTitle(): string {
    return 'Create evohome device';
  }

  protected getDialogButtons() {
    return {
      'OK': () => {
        const bValid = true;
        this.close();
        const SensorType = this.sensortype;
        if (typeof SensorType === 'undefined') {
          bootbox.alert(this.translationService.t('No Sensor Type Selected!'));
          return;
        }
        this.hardwareService.createEvohomeSensor(this.idx, SensorType).subscribe((data) => {
          if (data.status === 'OK') {
            this.notificationService.ShowNotify(this.translationService.t('Sensor Created, and can be found in the devices tab!'), 2500);
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
