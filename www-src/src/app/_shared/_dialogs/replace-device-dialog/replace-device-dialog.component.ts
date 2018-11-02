import {DeviceService} from '../../_services/device.service';
import {Component, OnInit, Input, AfterViewInit, Inject} from '@angular/core';
import {DialogComponent} from '../dialog.component';
import {DIALOG_DATA, DialogService} from '../../_services/dialog.service';
import {ITranslationService, I18NEXT_SERVICE} from 'angular-i18next';
import {TransferableItem} from 'src/app/_shared/_models/transfers';
import {NotificationService} from '../../_services/notification.service';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-replace-device-dialog',
  templateUrl: './replace-device-dialog.component.html',
  styleUrls: ['./replace-device-dialog.component.css']
})
export class ReplaceDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  devIdx: string;
  newDevices: Array<TransferableItem>;
  callbackFn?: () => any;

  newDeviceIdx = '';

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    private notificationService: NotificationService,
    @Inject(DIALOG_DATA) private data: any) {

    super(dialogService);

    this.devIdx = data.devIdx;
    this.newDevices = data.newDevices;
    this.callbackFn = data.callbackFn;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-replacedevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Replace Device');
  }

  protected getDialogButtons(): any {
    const dialog_replacedevice_buttons = {};

    dialog_replacedevice_buttons['OK'] = () => {
      this.close();

      if (this.newDeviceIdx === '') {
        const msg = this.translationService.t('No New Device Selected!');
        bootbox.alert(msg);
        return;
      }

      this.deviceService.transferDevice(this.devIdx, this.newDeviceIdx).subscribe(response => {
        if (response.status === 'OK') {
          if (this.callbackFn() !== undefined) {
            this.callbackFn();
          }
        } else {
          this.notificationService.ShowNotify(this.translationService.t('Problem Transfering Device!'), 2500, true);
        }
      }, error => {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Problem Transfering Device!'), 2500, true);
      });

    };

    dialog_replacedevice_buttons['Cancel'] = () => {
      this.close();
    };

    return dialog_replacedevice_buttons;
  }

}
