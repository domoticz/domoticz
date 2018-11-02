import {AfterViewInit, Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from 'src/app/_shared/_dialogs/dialog.component';
import {DIALOG_DATA, DialogService} from 'src/app/_shared/_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ConfigService} from '../../_services/config.service';
import {mergeMap} from 'rxjs/operators';
import {Device} from '../../_models/device';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-edit-setpoint-device-dialog',
  templateUrl: './edit-setpoint-device-dialog.component.html',
  styleUrls: ['./edit-setpoint-device-dialog.component.css']
})
export class EditSetpointDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  // FIXME replace those callbacks with a more Angular way to do so
  removeCallbackFn: () => any;
  updateCallbackFn: (t: Device) => any;

  tempsign: string;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    private configService: ConfigService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.item = data.item;
    this.removeCallbackFn = data.removeCallbackFn;
    this.updateCallbackFn = data.updateCallbackFn;
  }

  ngOnInit() {
    this.tempsign = this.configService.config.TempSign;
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-editsetpointdevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Device');
  }

  protected getDialogButtons(): any {
    const dialog_editsetpointdevice_buttons = {};

    dialog_editsetpointdevice_buttons[this.translationService.t('Update')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      if (bValid) {
        this.close();
        this.deviceService.updateSetPointDevice(this.item.idx, this.item.Name, this.item.Description,
          this.item.SetPoint, this.item.Protected).pipe(
            mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
          ).subscribe(updatedItem => {
            this.updateCallbackFn(updatedItem);
          });
      }
    };

    dialog_editsetpointdevice_buttons[this.translationService.t('Remove Device')] = () => {
      this.close();
      bootbox.confirm(this.translationService.t('Are you sure to remove this Device?'), (result) => {
        if (result === true) {
          this.deviceService.removeDevice(this.item.idx, this.item.Name, this.item.Description).subscribe(() => {
            this.removeCallbackFn();
          });
        }
      });
    };

    dialog_editsetpointdevice_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editsetpointdevice_buttons;
  }

}
