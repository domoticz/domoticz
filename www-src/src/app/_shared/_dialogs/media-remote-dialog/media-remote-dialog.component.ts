import { Component, OnInit, AfterViewInit, Inject } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { LightSwitchesService } from '../../_services/light-switches.service';
import { ConfigService } from '../../_services/config.service';
import { NotyHelper } from 'src/app/_shared/_utils/noty-helper';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-media-remote-dialog',
  templateUrl: './media-remote-dialog.component.html',
  styleUrls: ['./media-remote-dialog.component.css']
})
export class MediaRemoteDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  Name: string;
  devIdx: string;
  HWType: string;

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
    this.HWType = data.HWType;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-media-remote';
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

  public clickMediaRemote(action: string) {
    if (this.devIdx.length > 0) {
      if (this.HWType.indexOf('Kodi') >= 0) {
        this.lightSwitchesServices.mediaRemote('kodimediacommand', this.devIdx, action).subscribe(() => {
          // this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Sent remote command</b>', 100);
        }, () => {
          this.configService.cachenoty = NotyHelper.generate_noty('error', '<b>Problem sending remote command</b>', 1000);
        });
      } else if (this.HWType.indexOf('Panasonic') >= 0) {
        this.lightSwitchesServices.mediaRemote('panasonicmediacommand', this.devIdx, action).subscribe(() => {
          // this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Sent remote command</b>', 100);
        }, () => {
          this.configService.cachenoty = NotyHelper.generate_noty('error', '<b>Problem sending remote command</b>', 1000);
        });
      } else {
        this.configService.cachenoty = NotyHelper.generate_noty('error', '<b>Device Hardware is unknown.</b>', 1000);
      }
    } else {
      this.configService.cachenoty = NotyHelper.generate_noty('error', '<b>Device Index is unknown.</b>', 1000);
    }
  }

}
