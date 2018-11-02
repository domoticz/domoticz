import { Component, OnInit, AfterViewInit, Inject, OnDestroy } from '@angular/core';
import { DialogComponent } from 'src/app/_shared/_dialogs/dialog.component';
import { Subject, Observable, timer, Subscription } from 'rxjs';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import {Device} from "../../_models/device";

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-camera-live-dialog',
  templateUrl: './camera-live-dialog.component.html',
  styleUrls: ['./camera-live-dialog.component.css']
})
export class CameraLiveDialogComponent extends DialogComponent implements OnInit, AfterViewInit, OnDestroy {

  private item: { Name: string, CameraIdx: string };

  private width: number;
  private height: number;

  camfeedWidth = 600;
  camfeedHeight = 337;

  imageSrc = 'images/camera_default.png';

  private count = 0;

  timer: Subscription;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.item = data.item;

    const windowWidth = $(window).width() - 20;
    const windowHeight = $(window).height() - 150;

    const AspectSource = 4 / 3;

    this.height = windowHeight;
    // tslint:disable-next-line:no-bitwise
    this.width = Math.round(this.height * AspectSource) & ~1;
    if (this.width > windowWidth) {
      this.width = windowWidth;
      // tslint:disable-next-line:no-bitwise
      this.height = Math.round(this.width / AspectSource) & ~1;
    }

    // Set inner Camera feed width/height
    this.camfeedWidth = this.width - 30;
    this.camfeedHeight = this.height - 16;
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
  }

  ngOnDestroy() {
    if (this.timer) {
      this.timer.unsubscribe();
    }
  }

  protected getDialogId(): string {
    return 'dialog-camera-live';
  }

  protected getDialogTitle(): string {
    return this.item.Name;
  }

  protected getDialogButtons(): any {
    return {};
  }

  protected getDialogOptions(): any {
    return {
      width: this.width + 2,
      height: this.height + 50,
      position: {
        my: 'center',
        at: 'center',
        of: window
      }
    };
  }

  protected onOpen() {
    this.load_cam_video();
  }

  load_cam_video() {
    this.reload_cam_image();
    // FIXME why only one reload? should be interval rather than timeout, nope?
    this.timer = timer(100).subscribe(() => {
      this.reload_cam_image();
    });
  }

  private reload_cam_image() {
    const camfeed = 'camsnapshot.jpg?idx=' + this.item.CameraIdx;
    this.imageSrc = camfeed + '&count=' + this.count + '?t=' + new Date().getTime();
    this.count++;
  }

}
