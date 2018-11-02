import {AfterViewInit, Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from 'src/app/_shared/_dialogs/dialog.component';
import {DIALOG_DATA, DialogService} from 'src/app/_shared/_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ReplaceDeviceService} from 'src/app/_shared/_services/replace-device.service';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {CustomLightIcon} from 'src/app/_shared/_models/custom-light-icons';
import {Utils} from 'src/app/_shared/_utils/utils';
import {mergeMap} from 'rxjs/operators';
import {Device} from '../../_models/device';

// FIXME manage with NPM+TypeScript
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-edit-custom-sensor-device-dialog',
  templateUrl: './edit-custom-sensor-device-dialog.component.html',
  styleUrls: ['./edit-custom-sensor-device-dialog.component.css']
})
export class EditCustomSensorDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  // FIXME replace those callbacks with a more Angular way to do so
  replaceCallbackFn: () => any;
  removeCallbackFn: () => any;
  updateCallbackFn: (t: Device) => any;

  icons: Array<CustomLightIcon>;

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
  }

  ngAfterViewInit() {
    this.deviceService.getCustomLightIcons().subscribe(icons => {
      this.icons = icons;

      // FIXME remove this ddslick thing
      $('#combosensoricon').ddslick({
        data: icons,
        width: 260,
        height: 390,
        selectText: 'Sensor Icon',
        imagePosition: 'left'
      });

      // find our custom image index and select it
      icons.forEach((item: CustomLightIcon, i: number) => {
        if (item.value === this.item.CustomImage) {
          // FIXME remove ddslick
          $('#combosensoricon').ddslick('select', {index: i});
        }
      });
    });
  }

  protected getDialogId(): string {
    return 'dialog-editcustomsensordevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit Device');
  }

  protected getDialogButtons(): any {
    const dialog_editcustomsensordevice_buttons = {};

    dialog_editcustomsensordevice_buttons[this.translationService.t('Update')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      bValid = bValid && Utils.checkLength(this.item.SensorUnit, 1, 100);
      if (bValid) {
        const soptions = this.item.SensorType + ';' + encodeURIComponent(this.item.SensorUnit);
        const cval = $('#combosensoricon').data('ddslick').selectedIndex;
        const CustomImage = this.icons[cval].value;

        this.close();

        this.deviceService.updateCustomSensorDevice(this.item.idx, this.item.Name, this.item.Description, CustomImage, soptions)
          .pipe(
            // This can also update other fields of the item, so need to re get it
            mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
          )
          .subscribe(updatedItem => {
            this.updateCallbackFn(updatedItem);
          });

      }
    };

    dialog_editcustomsensordevice_buttons[this.translationService.t('Remove Device')] = () => {
      this.close();
      bootbox.confirm(this.translationService.t('Are you sure to remove this Device?'), (result) => {
        if (result === true) {
          this.deviceService.removeDevice(this.item.idx, this.item.Name, this.item.Description).subscribe(() => {
            this.removeCallbackFn();
          });
        }
      });
    };

    dialog_editcustomsensordevice_buttons[this.translationService.t('Replace')] = () => {
      this.close();
      this.replaceDeviceService.ReplaceDevice(this.item.idx, this.replaceCallbackFn);
    };

    dialog_editcustomsensordevice_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editcustomsensordevice_buttons;
  }

}
