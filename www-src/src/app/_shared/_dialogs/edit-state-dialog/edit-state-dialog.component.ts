import {DeviceService} from '../../_services/device.service';
import {AfterViewInit, Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from '../dialog.component';
import {DIALOG_DATA, DialogService} from '../../_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Utils} from '../../_utils/utils';
import {Device} from '../../_models/device';


@Component({
  selector: 'dz-edit-state-dialog',
  templateUrl: './edit-state-dialog.component.html',
  styleUrls: ['./edit-state-dialog.component.css']
})
export class EditStateDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  item: Device;
  updateCallbackFn: (t: Device) => any;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.item = data.item;
    this.updateCallbackFn = data.updateCallbackFn;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-editstate';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Edit State');
  }

  protected getDialogButtons(): any {
    const dialog_editstate_buttons = {};

    dialog_editstate_buttons[this.translationService.t('Set')] = () => {
      let bValid = true;
      bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);
      if (bValid) {
        this.close();

        let tUntil = '';
        if (this.item.Until !== '') {
          tUntil = new Date(this.item.Until).toISOString();
        }

        this.deviceService.updateState(this.item.idx, this.item.Name, this.item.Description, this.item.State,
          ((tUntil !== '') ? 'TemporaryOverride' : 'PermanentOverride'), tUntil)
          .subscribe(() => {
            this.updateCallbackFn(this.item);
          });
      }
    };

    dialog_editstate_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_editstate_buttons;
  }

  public MakePerm() {
    this.item.Until = '';
  }

}
