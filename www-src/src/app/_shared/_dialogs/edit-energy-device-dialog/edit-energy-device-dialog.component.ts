import {AfterViewInit, Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from 'src/app/_shared/_dialogs/dialog.component';
import {DIALOG_DATA, DialogService} from 'src/app/_shared/_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ReplaceDeviceService} from 'src/app/_shared/_services/replace-device.service';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {Device} from '../../_models/device';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-edit-energy-device-dialog',
  templateUrl: './edit-energy-device-dialog.component.html',
  styleUrls: ['./edit-energy-device-dialog.component.css']
})
export class EditEnergyDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  // FIXME replace those callbacks with a more Angular way to do so
  replaceCallbackFn: () => any;
  removeCallbackFn: () => any;
  updateCallbackFn: (t: Device) => any;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private replaceDeviceService: ReplaceDeviceService,
    private deviceService: DeviceService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.item = data.item;
    this.replaceCallbackFn = data.replaceCallbackFn;
    this.removeCallbackFn = data.removeCallbackFn;
    this.updateCallbackFn = data.updateCallbackFn;
  }

  ngOnInit() {
    if (this.item.EnergyMeterMode === undefined || this.item.EnergyMeterMode === '') {
      this.item.EnergyMeterMode = '0';
    }
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-editenergydevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Device');
  }

  protected getDialogButtons(): any {
    const dialog_editenergydevice_buttons = {};

    dialog_editenergydevice_buttons[this.translationService.t('Update')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      if (bValid) {
        this.close();
        this.deviceService.updateEnergyDevice(this.item.idx, this.item.Name, this.item.Description,
          this.item.SwitchTypeVal, this.item.EnergyMeterMode).subscribe(() => {
            this.updateCallbackFn(this.item);
          });
      }
    };

    dialog_editenergydevice_buttons[this.translationService.t('Remove Device')] = () => {
      this.close();
      bootbox.confirm(this.translationService.t('Are you sure to remove this Device?'), (result) => {
        if (result === true) {
          this.deviceService.removeDevice(this.item.idx, this.item.Name, this.item.Description).subscribe(() => {
            this.removeCallbackFn();
          });
        }
      });
    };

    dialog_editenergydevice_buttons[this.translationService.t('Replace')] = () => {
      this.close();
      this.replaceDeviceService.ReplaceDevice(this.item.idx, this.replaceCallbackFn);
    };

    dialog_editenergydevice_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editenergydevice_buttons;
  }

}
