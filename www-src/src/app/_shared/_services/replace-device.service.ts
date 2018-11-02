import { DeviceService } from './device.service';
import { ReplaceDeviceDialogComponent } from '../_dialogs/replace-device-dialog/replace-device-dialog.component';
import { DialogService } from './dialog.service';
import { Injectable, Inject } from '@angular/core';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Injectable()
export class ReplaceDeviceService {

    private dialog: ReplaceDeviceDialogComponent;

    constructor(private dialogService: DialogService,
        private deviceService: DeviceService,
        @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {
    }

    public ReplaceDevice(idx: string, backfunction?: () => any) {
        this.deviceService.getTransfers(idx).subscribe(transfers => {
            if (!transfers.result) {
                bootbox.alert(this.translationService.t('No devices to Transfer too!'));
            } else {
                const data = {
                    devIdx: idx,
                    newDevices: transfers.result,
                    callbackFn: backfunction
                };
                this.dialog = this.dialogService.addDialog(ReplaceDeviceDialogComponent, data).instance;
                this.dialog.init();
                this.dialog.open();
            }
        });
    }

}
