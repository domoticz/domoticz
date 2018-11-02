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
  selector: 'dz-edit-visibility-device-dialog',
  templateUrl: './edit-visibility-device-dialog.component.html',
  styleUrls: ['./edit-visibility-device-dialog.component.css']
})
export class EditVisibilityDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
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
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-editvisibilitydevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Device');
  }

  protected getDialogButtons(): any {
    const dialog_editvisibilitydevice_buttons = {};

    dialog_editvisibilitydevice_buttons[this.translationService.t('Update')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      if (bValid) {
        this.close();
        // When changing the SwitchTypeVal, other attributes of the item must be recomputed, that's why we call the API to get
        // the updated item instead of just returning this.item
        this.deviceService.updateVisibilityDevice(this.item.idx, this.item.Name, this.item.Description, this.item.SwitchTypeVal).pipe(
          mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
        ).subscribe((updatedItem: Device) => {
          this.updateCallbackFn(updatedItem);
        });
      }
    };

    dialog_editvisibilitydevice_buttons[this.translationService.t('Remove Device')] = () => {
      this.close();
      bootbox.confirm(this.translationService.t('Are you sure to remove this Device?'), (result) => {
        if (result === true) {
          this.deviceService.removeDevice(this.item.idx, this.item.Name, this.item.Description).subscribe(() => {
            this.removeCallbackFn();
          });
        }
      });
    };

    dialog_editvisibilitydevice_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editvisibilitydevice_buttons;
  }

}
