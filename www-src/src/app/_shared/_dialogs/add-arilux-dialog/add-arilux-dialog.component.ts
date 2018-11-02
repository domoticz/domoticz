import { Component, OnInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { HardwareService } from 'src/app/_shared/_services/hardware.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-add-arilux-dialog',
  templateUrl: './add-arilux-dialog.component.html',
  styleUrls: ['./add-arilux-dialog.component.css']
})
export class AddAriluxDialogComponent extends DialogComponent implements OnInit {

  private idx: string;

  name: string;
  ipaddress: string;
  lighttype: string;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    @Inject(DIALOG_DATA) data: any,
    private notificationService: NotificationService,
    private hardwareService: HardwareService) {

    super(dialogService);

    this.idx = data.idx;
  }

  ngOnInit() {
  }

  protected getDialogOptions(): any {
    return {
      width: 420,
      height: 250
    };
  }

  protected getDialogId(): string {
    return 'dialog-addArilux';
  }

  protected getDialogTitle(): string {
    return 'Add Arilux AL-C0x controller';
  }

  protected getDialogButtons() {
    return {
      'OK': () => {
        const bValid = true;
        const SensorName = this.name;
        if (SensorName === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
          return;
        }
        const IPAddress = this.ipaddress;
        if (IPAddress === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter a IP Address!'), 2500, true);
          return;
        }
        const SensorType = this.lighttype;
        if (typeof SensorType === 'undefined') {
          bootbox.alert(this.translationService.t('No Light Type Selected!'));
          return;
        }

        this.close();

        this.hardwareService.addArilux(this.idx, SensorName, IPAddress, SensorType).subscribe((data) => {
          if (data.status === 'OK') {
            this.notificationService.ShowNotify(this.translationService.t('Light created, and can be found in the devices tab!'), 2500);
          } else {
            this.notificationService.ShowNotify(this.translationService.t('Problem adding Light!'), 2500, true);
          }
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem adding Light!'), 2500, true);
        });
      },
      Cancel: () => {
        this.close();
      }
    };
  }

}
