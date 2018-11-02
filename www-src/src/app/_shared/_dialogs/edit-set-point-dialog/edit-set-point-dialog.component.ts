import {DeviceService} from '../../_services/device.service';
import {AfterViewInit, Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from '../dialog.component';
import {DIALOG_DATA, DialogService} from '../../_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ReplaceDeviceService} from '../../_services/replace-device.service';
import {Utils} from '../../_utils/utils';
import {Device} from '../../_models/device';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-edit-set-point-dialog',
  templateUrl: './edit-set-point-dialog.component.html',
  styleUrls: ['./edit-set-point-dialog.component.css']
})
export class EditSetPointDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  updateCallbackFn: (t: Device) => any;
  displayCancelOverride: boolean;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private replaceDeviceService: ReplaceDeviceService,
    private deviceService: DeviceService,
    @Inject(DIALOG_DATA) private data: any) {

    super(dialogService);

    this.item = data.item;
    this.updateCallbackFn = data.updateCallbackFn;
    this.displayCancelOverride = data.displayCancelOverride;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-editsetpoint';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Set Point');
  }

  protected getDialogButtons(): any {
    const dialog_editsetpoint_buttons = {};

    dialog_editsetpoint_buttons[this.translationService.t('Set')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      const setpoint = this.item.SetPoint;
      if (setpoint < 5 || setpoint > 35) {
        bootbox.alert(this.translationService.t('Set point must be between 5 and 35 degrees'));
        return false;
      }
      let tUntil = '';
      if (this.item.Until !== '') {
        const selectedDate = new Date(this.item.Until);
        const now = new Date();
        if (selectedDate < now) {
          bootbox.alert(this.translationService.t('Temporary set point date / time must be in the future'));
          return false;
        }
        tUntil = Utils.ConvertTimeWithTimeZoneOffset(selectedDate);
      }
      if (bValid) {
        this.close();

        this.deviceService.updateSetPoint(this.item.idx, this.item.Name, this.item.Description, setpoint,
          ((tUntil !== '') ? 'TemporaryOverride' : 'PermanentOverride'), tUntil)
          .subscribe(response => {
            this.updateCallbackFn(this.item);
          });
      }
    };

    if (this.displayCancelOverride) {
      dialog_editsetpoint_buttons[this.translationService.t('Cancel Override')] = () => {
        let bValid = true;
        bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
        if (bValid) {
          this.close();
          let aValue = this.item.SetPoint;
          if (aValue < 5) {
            aValue = 5; // These values will display but the controller will update back the currently scheduled setpoint in due course
          }
          if (aValue > 35) {
            aValue = 35; // These values will display but the controller will update back the currently scheduled setpoint in due course
          }

          this.deviceService.updateSetPointAuto(this.item.idx, this.item.Name, this.item.Description, aValue).subscribe(response => {
            this.updateCallbackFn(this.item);
          });
        }
      };
    }

    dialog_editsetpoint_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editsetpoint_buttons;
  }

  public MakePerm() {
    this.item.Until = '';
  }

}
