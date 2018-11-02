import { Component, OnInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { HardwareService } from '../../_services/hardware.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-create-dummy-sensor-dialog',
  templateUrl: './create-dummy-sensor-dialog.component.html',
  styleUrls: ['./create-dummy-sensor-dialog.component.css']
})
export class CreateDummySensorDialogComponent extends DialogComponent implements OnInit {

  private idx: string;
  private name: string;

  displayvsensoraxis = false;
  sensorname: string;
  sensortype: string;
  sensoraxis = '';

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    @Inject(DIALOG_DATA) data: any,
    private notificationService: NotificationService,
    private hardwareService: HardwareService) {

    super(dialogService);

    this.idx = data.idx;
    this.name = data.name;
  }

  ngOnInit() {
  }

  onOpen() {
    this.OnDummySensorTypeChange();
  }

  protected getDialogOptions(): any {
    return {
      width: 420,
      height: 250
    };
  }

  protected getDialogId(): string {
    return 'dialog-createsensor';
  }

  protected getDialogTitle(): string {
    return 'Create Virtual Sensor';
  }

  protected getDialogButtons() {
    return {
      'OK': () => {
        const bValid = true;
        this.close();

        const SensorName = this.sensorname;
        if (SensorName === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
          return;
        }
        const SensorType = this.sensortype;
        if (typeof SensorType === 'undefined') {
          bootbox.alert(this.translationService.t('No Sensor Type Selected!'));
          return;
        }
        let sensoroptions;
        if (SensorType === '0xF31F') {
          const AxisLabel = this.sensoraxis;
          if (AxisLabel === '') {
            this.notificationService.ShowNotify(this.translationService.t('Please enter a Axis Label!'), 2500, true);
            return;
          }
          sensoroptions = '1;' + AxisLabel;
        }

        this.hardwareService.createDevice(this.idx, SensorName, SensorType, sensoroptions).subscribe((data) => {
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

  OnDummySensorTypeChange() {
    const stype = this.sensortype;
    this.sensoraxis = '';
    if (stype === '0xF31F') {
      this.displayvsensoraxis = true;
    } else {
      this.displayvsensoraxis = false;
    }
  }

}
