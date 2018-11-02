import {Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from '../dialog.component';
import {DIALOG_DATA, DialogService} from '../../_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from '../../_services/notification.service';

@Component({
  selector: 'dz-add-edit-floorplan-dialog',
  templateUrl: './add-edit-floorplan-dialog.component.html',
  styleUrls: ['./add-edit-floorplan-dialog.component.css']
})
export class AddEditFloorplanDialogComponent extends DialogComponent implements OnInit {

  editMode = false;

  floorplanname = '';
  scalefactor = '1.0';
  imagefile = '';

  file: File;

  callbackFn: (Settings?) => void;

  constructor(dialogService: DialogService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              @Inject(DIALOG_DATA) data: any,
              private notificationService: NotificationService) {
    super(dialogService);

    this.floorplanname = data.floorplanname;
    this.scalefactor = data.scalefactor;
    this.editMode = data.editMode;
    if (!this.editMode && data.imagefile) {
      this.imagefile = data.imagefile;
    }
    this.callbackFn = data.callbackFn;
  }

  ngOnInit() {
  }

  protected getDialogId(): string {
    return 'dialog-add-edit-floorplan';
  }

  protected getDialogTitle(): string {
    return this.editMode ? 'Edit Floorplan' : 'Add New Floorplan';
  }

  protected getDialogOptions(): any {
    if (!this.editMode) {
      return {
        width: '500',
      };
    } else {
      return {};
    }
  }

  protected getDialogButtons(): any {
    return {
      'Cancel': () => {
        this.close();
      },
      'Add': () => {
        const csettings = this.GetFloorplanSettings(!this.editMode);
        if (typeof csettings === 'undefined') {
          return;
        }
        this.close();
        this.callbackFn(csettings);
      }
    };
  }

  private GetFloorplanSettings(isNew: boolean): Settings | undefined {
    const csettings = {
      name: this.floorplanname,
      scalefactor: this.scalefactor,
      file: this.file
    };

    if (csettings.name === '') {
      this.notificationService.ShowNotify('Please enter a Name!', 2500, true);
      return;
    }
    if (Number(csettings.scalefactor) === undefined) {
      this.notificationService.ShowNotify(this.translationService.t('Icon Scale can only contain numbers...'), 2000, true);
      return;
    }
    if (isNew) {
      if (this.imagefile === '') {
        this.notificationService.ShowNotify('Please choose an image filename!', 2500, true);
        return;
      }
    }
    return csettings;
  }

  onFileChange(event: any) {
    if (event.target.files && event.target.files.length > 0) {
      this.file = event.target.files[0];
    }
  }

}

interface Settings {
  name: string;
  scalefactor: string;
  file: File;
}
