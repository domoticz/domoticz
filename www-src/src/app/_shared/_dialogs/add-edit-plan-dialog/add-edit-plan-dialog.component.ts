import {Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from '../dialog.component';
import {DIALOG_DATA, DialogService} from '../../_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from '../../_services/notification.service';

@Component({
  selector: 'dz-add-edit-plan-dialog',
  templateUrl: './add-edit-plan-dialog.component.html',
  styleUrls: ['./add-edit-plan-dialog.component.css']
})
export class AddEditPlanDialogComponent extends DialogComponent implements OnInit {

  planname = '';

  private callbackFn: (name: string) => void;
  private title = 'Add New Plan';
  private buttonTitle = 'Add';

  constructor(dialogService: DialogService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              @Inject(DIALOG_DATA) data: any,
              private notificationService: NotificationService) {
    super(dialogService);

    this.title = data.title;
    this.buttonTitle = data.buttonTitle;
    this.callbackFn = data.callbackFn;
    if (data.name) {
      this.planname = data.name;
    }
  }

  ngOnInit() {
  }

  protected getDialogId(): string {
    return 'dialog-add-edit-plan';
  }

  protected getDialogTitle(): string {
    return this.translationService.t(this.title);
  }

  protected getDialogOptions(): any {
    return {
      width: 390,
      height: 200
    };
  }

  protected getDialogButtons(): any {
    const buttons = {
      'Cancel': () => {
        this.close();
      }
    };
    buttons[this.buttonTitle] = () => {
      if (this.planname === '') {
        this.notificationService.ShowNotify('Please enter a Name!', 2500, true);
        return;
      }
      this.close();
      this.callbackFn(this.planname);
    };
    return buttons;
  }

}
