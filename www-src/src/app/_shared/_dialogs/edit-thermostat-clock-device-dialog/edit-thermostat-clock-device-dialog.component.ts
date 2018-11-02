import {AfterViewInit, Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from 'src/app/_shared/_dialogs/dialog.component';
import {DIALOG_DATA, DialogService} from 'src/app/_shared/_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {mergeMap} from 'rxjs/operators';
import {Device} from '../../_models/device';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-edit-thermostat-clock-device-dialog',
  templateUrl: './edit-thermostat-clock-device-dialog.component.html',
  styleUrls: ['./edit-thermostat-clock-device-dialog.component.css']
})
export class EditThermostatClockDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  day: number;
  hour: string;
  minute: string;
  // FIXME replace those callbacks with a more Angular way to do so
  removeCallbackFn: () => any;
  updateCallbackFn: (t: Device) => any;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.item = data.item;
    this.removeCallbackFn = data.removeCallbackFn;
    this.updateCallbackFn = data.updateCallbackFn;
  }

  ngOnInit() {
    const sarray = this.item.DayTime.split(';');
    this.day = Number(sarray[0]);
    this.hour = sarray[1];
    this.minute = sarray[2];
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-editthermostatclockdevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Device');
  }

  protected getDialogButtons(): any {
    const dialog_editthermostatclockdevice_buttons = {};

    dialog_editthermostatclockdevice_buttons[this.translationService.t('Update')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      if (bValid) {
        this.close();
        bootbox.alert(this.translationService.t('Setting the Clock is not finished yet!'));
        const daytimestr = this.day + ';' + this.hour + ';' + this.minute;
        this.deviceService.updateThermostatClockDevice(this.item.idx, this.item.Name, this.item.Description,
          daytimestr, this.item.Protected)
          .pipe(
            mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
          )
          .subscribe((updatedItem) => {
            this.updateCallbackFn(updatedItem);
          });
      }
    };

    dialog_editthermostatclockdevice_buttons[this.translationService.t('Remove Device')] = () => {
      this.close();
      bootbox.confirm(this.translationService.t('Are you sure to remove this Device?'), (result) => {
        if (result === true) {
          this.deviceService.removeDevice(this.item.idx, this.item.Name, this.item.Description);
        }
      });
    };

    dialog_editthermostatclockdevice_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editthermostatclockdevice_buttons;
  }

}
