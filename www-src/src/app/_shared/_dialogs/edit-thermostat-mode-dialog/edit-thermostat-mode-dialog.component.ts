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
  selector: 'dz-edit-thermostat-mode-dialog',
  templateUrl: './edit-thermostat-mode-dialog.component.html',
  styleUrls: ['./edit-thermostat-mode-dialog.component.css']
})
export class EditThermostatModeDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  // FIXME replace those callbacks with a more Angular way to do so
  removeCallbackFn: () => any;
  updateCallbackFn: (t: Device) => any;

  options: Array<[string, string]> = [];

  private isFan: boolean;

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
    const sarray = this.item.Modes.split(';');
    // populate mode combo
    let ii = 0;
    while (ii < sarray.length - 1) {
      this.options.push([sarray[ii], sarray[ii + 1]]);
      ii += 2;
    }

    this.isFan = this.item.SubType === 'Thermostat Fan Mode';
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-editthermostatmode';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Device');
  }

  protected getDialogButtons(): any {
    const dialog_editthermostatmode_buttons = {};

    dialog_editthermostatmode_buttons[this.translationService.t('Update')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      if (bValid) {
        this.close();
        this.deviceService.updateThermostatModeDevice(this.item.idx, this.item.Name, this.item.Description,
          this.isFan, this.item.Mode, this.item.Protected)
          .pipe(
            mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
          ).subscribe(updatedItem => {
            this.updateCallbackFn(updatedItem);
          });
      }
    };

    dialog_editthermostatmode_buttons[this.translationService.t('Remove Device')] = () => {
      this.close();
      bootbox.confirm(this.translationService.t('Are you sure to remove this Device?'), (result) => {
        if (result === true) {
          this.deviceService.removeDevice(this.item.idx, this.item.Name, this.item.Description).subscribe(() => {
            this.removeCallbackFn();
          });
        }
      });
    };

    dialog_editthermostatmode_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editthermostatmode_buttons;
  }

}
