import { Component, OnInit, AfterViewInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { Utils } from 'src/app/_shared/_utils/utils';
import { SceneService } from '../../_services/scene.service';
import { NotificationService } from '../../_services/notification.service';

@Component({
  selector: 'dz-add-scene-dialog',
  templateUrl: './add-scene-dialog.component.html',
  styleUrls: ['./add-scene-dialog.component.css']
})
export class AddSceneDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  // FIXME replace those callbacks with a more Angular way to do so
  addedCallbackFn: () => any;

  name: string;
  type = '0';

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private sceneService: SceneService,
    private notificationService: NotificationService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.addedCallbackFn = data.addedCallbackFn;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogOptions() {
    return {
      width: 380,
      height: 200
    };
  }

  protected getDialogId(): string {
    return 'dialog-addscene';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Add Scene');
  }

  protected getDialogButtons(): any {

    return [{
      text: this.translationService.t('Add Scene'),
      click: () => {
        let bValid = true;
        bValid = bValid && Utils.checkLength(this.name, 2, 100);
        if (bValid) {
          this.sceneService.addScene(this.name, this.type).subscribe(response => {
            if (typeof response.status !== 'undefined') {
              if (response.status === 'OK') {
                this.close();
                this.addedCallbackFn();
              } else {
                this.notificationService.ShowNotify(response.message, 3000, true);
              }
            }
          });
        }
      }
    }, {
      text: this.translationService.t('Cancel'),
      click: () => {
        this.close();
      }
    }];
  }
}
