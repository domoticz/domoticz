import { DeviceService } from '../../_services/device.service';
import { Component, OnInit, Input, AfterViewInit, Inject } from '@angular/core';
import { DialogComponent } from '../dialog.component';
import { DIALOG_DATA, DialogService } from '../../_services/dialog.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { ReplaceDeviceService } from '../../_services/replace-device.service';
import { Utils } from '../../_utils/utils';
import {Device} from "../../_models/device";

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-edit-temp-device-dialog',
  templateUrl: './edit-temp-device-dialog.component.html',
  styleUrls: ['./edit-temp-device-dialog.component.css']
})
export class EditTempDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  tempsign: string;
  displayAddj: boolean;
  // FIXME replace those callbacks with a more Angular way to do so
  replaceCallbackFn: () => any;
  removeCallbackFn: () => any;
  updateCallbackFn: (t: Device) => any;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private replaceDeviceService: ReplaceDeviceService,
    private deviceService: DeviceService,
    @Inject(DIALOG_DATA) private data: any) {

    super(dialogService);

    this.item = data.item;
    this.tempsign = data.tempsign;
    this.replaceCallbackFn = data.replaceCallbackFn;
    this.removeCallbackFn = data.removeCallbackFn;
    this.updateCallbackFn = data.updateCallbackFn;
    this.displayAddj = data.displayAddj;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-eddittempdevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Device');
  }

  protected getDialogButtons(): any {
    const dialog_edittempdevice_buttons = {};

    dialog_edittempdevice_buttons[this.translationService.t('Update')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      if (bValid) {
        this.close();
        const addjValue = this.displayAddj ? this.item.AddjValue : undefined;
        this.deviceService.updateDevice(this.item.idx, this.item.Name, this.item.Description, addjValue).subscribe(response => {
          this.updateCallbackFn(this.item);
        });
      }
    };

    dialog_edittempdevice_buttons[this.translationService.t('Remove Device')] = () => {
      this.close();
      bootbox.confirm(this.translationService.t('Are you sure to remove this Device?'), result => {
        if (result === true) {
          this.deviceService.removeDevice(this.item.idx, this.item.Name, this.item.Description).subscribe(response => {
            this.removeCallbackFn();
          });
        }
      });
    };

    dialog_edittempdevice_buttons[this.translationService.t('Replace')] = () => {
      this.close();
      this.replaceDeviceService.ReplaceDevice(this.item.idx, this.replaceCallbackFn);
    };

    dialog_edittempdevice_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_edittempdevice_buttons;
  }

}
