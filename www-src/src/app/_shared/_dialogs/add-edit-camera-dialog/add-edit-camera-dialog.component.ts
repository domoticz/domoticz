import { Component, OnInit, Inject } from '@angular/core';
import { DialogComponent } from '../dialog.component';
import { DialogService, DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { CamService } from '../../_services/cam.service';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { Camera } from 'src/app/_shared/_models/camera';
import { Subject, timer } from 'rxjs';
import { Utils } from 'src/app/_shared/_utils/utils';

@Component({
  selector: 'dz-add-edit-camera-dialog',
  templateUrl: './add-edit-camera-dialog.component.html',
  styleUrls: ['./add-edit-camera-dialog.component.css']
})
export class AddEditCameraDialogComponent extends DialogComponent implements OnInit {

  camfeedSrc = 'images/camera_default.png';

  mode: 'add' | 'edit' = 'add';
  camera: CameraForm = {
    Address: '',
    Enabled: true,
    ImageURL: 'image.jpg',
    Name: '',
    Password: '',
    Port: '',
    Protocol: 0,
    Username: '',
    idx: ''
  };

  refresh = new Subject<boolean>();

  count: number;
  testcamfeed: string;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    @Inject(DIALOG_DATA) data: any,
    private notificationService: NotificationService,
    private camService: CamService) {

    super(dialogService);

    this.mode = data.mode;
    if (this.mode === 'edit') {
      const cam = data.camera as Camera;
      this.camera = {
        Address: cam.Address,
        Enabled: cam.Enabled === 'true',
        ImageURL: cam.ImageURL,
        Name: cam.Name,
        Password: cam.Password,
        Port: cam.Port.toString(),
        Protocol: cam.Protocol,
        Username: cam.Username,
        idx: cam.idx
      };
    }
  }

  ngOnInit() {
  }

  protected getDialogOptions(): any {
    return {
      width: 720,
      height: 480
    };
  }

  protected getDialogId(): string {
    return 'dialog-add-edit-camera';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Add New Camera');
  }

  protected getDialogButtons() {
    return {
      'Cancel': () => {
        this.close();
      },
      'Add': () => {
        const csettings = this.GetCameraSettings();
        if (typeof csettings === 'undefined') {
          return;
        }
        this.close();
        if (this.mode === 'add') {
          this.Addcamera();
        } else {
          this.Updatecamera();
        }
      }
    };
  }

  protected onOpen() {
    this.camfeedSrc = 'images/camera_default.png';
    this.count = 0;
    this.testcamfeed = '';
  }

  protected onClose() {
    this.camfeedSrc = 'images/camera_default.png';
    this.testcamfeed = '';
  }

  OnCameraProtocolChange() {
    const protocol = this.camera.Protocol;
    let port;
    if (protocol === 0) {
      // HTTP
      port = 80;
    } else {
      // HTTPS
      port = 443;
    }
    this.camera.Port = port;
  }

  GetCameraSettings(): Camera | undefined {
    if (this.camera.Name === '') {
      this.notificationService.ShowNotify('Please enter a Name!', 2500, true);
      return;
    }

    if (this.camera.Address === '') {
      this.notificationService.ShowNotify('Please enter an address!', 2500, true);
      return;
    }
    if (this.camera.Port !== '') {
      const intRegex = /^\d+$/;
      if (!intRegex.test(this.camera.Port)) {
        this.notificationService.ShowNotify('Please enter a valid Port!', 2500, true);
        return;
      }
    }

    if (this.camera.ImageURL === '') {
      this.notificationService.ShowNotify('Please enter a Image url!', 2500, true);
      return;
    }

    return {
      Address: this.camera.Address,
      Enabled: this.camera.Enabled.toString(),
      ImageURL: this.camera.ImageURL,
      Name: this.camera.Name,
      Password: this.camera.Password,
      Port: this.camera.Port === '' ? (this.camera.Protocol === 0 ? 80 : 443) : Number(this.camera.Port),
      Protocol: this.camera.Protocol,
      Username: this.camera.Username,
      idx: this.camera.idx
    };
  }

  Addcamera() {
    const csettings = this.GetCameraSettings();
    if (typeof csettings === 'undefined') {
      return;
    }

    this.camService.addCamera(csettings).subscribe(() => {
      this.refresh.next(true);
    }, error => {
      this.notificationService.ShowNotify('Problem adding camera!', 2500, true);
    });
  }

  Updatecamera() {
    const csettings = this.GetCameraSettings();
    if (typeof csettings === 'undefined') {
      return;
    }

    this.camService.updateCamera(csettings).subscribe(() => {
      this.refresh.next(true);
    }, error => {
      this.notificationService.ShowNotify('Problem updating camera settings!', 2500, true);
    });
  }

  TestCamFeed() {
    const csettings = this.GetCameraSettings();
    if (typeof csettings === 'undefined') {
      return;
    }
    const feedsrc = Utils.GenerateCamImageURL(csettings);
    this.testcamfeed = feedsrc;
    this.count = 0;
    this.camfeedSrc = feedsrc;
  }

  load_testcam_video() {
    if ((typeof this.testcamfeed === 'undefined') || (this.testcamfeed === '')) {
      return;
    }
    timer(100).subscribe(() => {
      this.reload_testcam_image();
    });
  }

  reload_testcam_image() {
    const xx = new Image();
    let src = this.testcamfeed;
    if (src.indexOf('?') !== -1) {
      src += '&count=';
    } else {
      src += '?count=';
    }
    src += this.count;
    xx.src = src;
    this.count++;
    this.camfeedSrc = xx.src;
  }

}

interface CameraForm {
  Address: string;
  Enabled: boolean;
  ImageURL: string;
  Name: string;
  Password: string;
  Port: string;
  Protocol: number;
  Username: string;
  idx: string;
}
