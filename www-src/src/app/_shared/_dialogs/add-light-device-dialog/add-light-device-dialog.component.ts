import { Component, OnInit, AfterViewInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { ReplaceDeviceService } from 'src/app/_shared/_services/replace-device.service';
import { DeviceService } from 'src/app/_shared/_services/device.service';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { Utils } from 'src/app/_shared/_utils/utils';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-add-light-device-dialog',
  templateUrl: './add-light-device-dialog.component.html',
  styleUrls: ['./add-light-device-dialog.component.css']
})
export class AddLightDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  // FIXME replace those callbacks with a more Angular way to do so
  addCallback: () => any;

  devIdx: string;

  devicename = '';
  switchtype: number;
  subdevice;
  how = 'New';

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private replaceDeviceService: ReplaceDeviceService,
    private deviceService: DeviceService,
    private notificationService: NotificationService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.devIdx = data.devIdx;
    this.addCallback = data.addCallback;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-addlightdevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Add Light/Switch Device');
  }

  protected getDialogOptions(): any {
    return {
      width: 400,
      height: 290
    };
  }

  protected getDialogButtons(): any {
    return {
      'Add Device': () => {
        let bValid = true;
        bValid = bValid && Utils.checkLength(this.devicename, 2, 100);
        const bIsSubDevice = this.how === 'Sub Device';
        let MainDeviceIdx = '';
        if (bIsSubDevice) {
          MainDeviceIdx = this.subdevice;
          if (typeof MainDeviceIdx === 'undefined') {
            bootbox.alert(this.translationService.t('No Main Device Selected!'));
            return;
          }
        }

        if (bValid) {
          this.close();
          this.deviceService.updateLightDevice(this.devIdx, this.devicename, this.switchtype, MainDeviceIdx).subscribe(data => {
            this.addCallback();
          });
        }
      },
      Cancel: () => {
        this.close();
      }
    };
  }

}
