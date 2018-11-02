import { Component, OnInit, AfterViewInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { LightSwitchesService } from 'src/app/_shared/_services/light-switches.service';
import { Subject, Observable } from 'rxjs';

@Component({
  selector: 'dz-arm-system-dialog',
  templateUrl: './arm-system-dialog.component.html',
  styleUrls: ['./arm-system-dialog.component.css']
})
export class ArmSystemDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  private _onArm = new Subject<string>();

  private meiantech: boolean;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.meiantech = data.meiantech;
  }

  get onArm(): Observable<string> {
    return this._onArm;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'arm-system-dialog';
  }

  protected getDialogTitle(): string {
    return '';
  }

  protected getDialogButtons(): any {
    const _this = this;
    if (!this.meiantech) {
      return [{
        text: this.translationService.t('Arm Home'),
        click: () => {
          _this._onArm.next('Arm Home');
          _this.close();
        }
      },
      {
        text: this.translationService.t('Arm Away'),
        click: () => {
          _this._onArm.next('Arm Away');
          _this.close();
        }
      }];
    } else {
      return [
        {
          text: this.translationService.t('Normal'),
          click: function () {
            _this._onArm.next('Normal');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Alarm'),
          click: function () {
            _this._onArm.next('Alarm');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Arm Home'),
          click: function () {
            _this._onArm.next('Arm Home');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Arm Home Delayed'),
          click: function () {
            _this._onArm.next('Arm Home Delayed');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Arm Away'),
          click: function () {
            _this._onArm.next('Arm Away');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Arm Away Delayed'),
          click: function () {
            _this._onArm.next('Arm Away Delayed');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Panic'),
          click: function () {
            _this._onArm.next('Panic');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Disarm'),
          click: function () {
            _this._onArm.next('Disarm');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Light On'),
          click: function () {
            _this._onArm.next('Light On');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Light Off'),
          click: function () {
            _this._onArm.next('Light Off');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Light 2 On'),
          click: function () {
            _this._onArm.next('Light 2 On');
            _this.close();
          }
        },
        {
          text: this.translationService.t('Light 2 Off'),
          click: function () {
            _this._onArm.next('Light 2 Off');
            _this.close();
          }
        }
      ];
    }
  }

  protected getDialogOptions(): any {
    return {
      width: this.meiantech ? 420 : 340,
      draggable: false
    };
  }

}
