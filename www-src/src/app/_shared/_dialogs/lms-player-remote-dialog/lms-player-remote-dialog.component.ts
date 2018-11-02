import { Component, OnInit, AfterViewInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { ConfigService } from 'src/app/_shared/_services/config.service';
import { LightSwitchesService } from 'src/app/_shared/_services/light-switches.service';
import { NotyHelper } from 'src/app/_shared/_utils/noty-helper';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-lms-player-remote-dialog',
  templateUrl: './lms-player-remote-dialog.component.html',
  styleUrls: ['./lms-player-remote-dialog.component.css']
})
export class LmsPlayerRemoteDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  Name: string;
  devIdx: string;

  viewBox = { x: 50, y: -150, width: 900, height: 1875 };

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private configService: ConfigService,
    private lightSwitchesServices: LightSwitchesService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.Name = data.Name;
    this.devIdx = data.devIdx;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-lmsplayer-remote';
  }

  protected getDialogTitle(): string {
    return this.translationService.t(this.Name);
  }

  protected getDialogButtons(): any {
    return {};
  }

  protected getDialogOptions(): any {
    // Need to make as big as possible so work out maximum height then set width appropriately
    const svgRatio = (this.viewBox.width - this.viewBox.x) / (this.viewBox.height - this.viewBox.y);
    const dheight = $(window).height() * 0.85;
    const dwidth = dheight * svgRatio;
    // for v2.0, if screen is wide enough add room to show media at the side of the remote
    return {
      show: 'blind',
      hide: 'blind',
      width: dwidth,
      height: dheight,
      position: { my: 'center', at: 'center', of: window },
      fluid: true,
    };
  }

  public clickLmsplayerRemote(action: string) {
    if (this.devIdx.length > 0) {
      this.lightSwitchesServices.mediaRemote('lmsmediacommand', this.devIdx, action).subscribe(() => {
        // Nothing
      }, error => {
        this.configService.cachenoty = NotyHelper.generate_noty('error', '<b>Problem sending remote command</b>', 1000);
      });
    } else {
      this.configService.cachenoty = NotyHelper.generate_noty('error', '<b>Device Index is unknown.</b>', 1000);
    }
  }

}
