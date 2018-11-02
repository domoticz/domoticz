import {Component, Inject, OnDestroy, OnInit} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {interval, Subscription} from 'rxjs';
import {UpdateService} from '../../_shared/_services/update.service';
import {Router} from '@angular/router';
import {ConfigService} from '../../_shared/_services/config.service';

@Component({
  selector: 'dz-update',
  templateUrl: './update.component.html',
  styleUrls: ['./update.component.css']
})
export class UpdateComponent implements OnInit, OnDestroy {

  appVersion = '0';
  newVersion = '0';

  topText = '';
  statusText = '';
  bottomText = '';

  displayProgress = false;
  progressDataPercentage = 0;

  updateReady = false;

  private mytimer: Subscription;
  private mytimer2: Subscription;

  constructor(private updateService: UpdateService,
              private configService: ConfigService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private router: Router) {
  }

  ngOnInit() {
    this.appVersion = this.configService.appversion.getValue();

    this.topText = this.translationService.t('Checking for updates....');
    this.bottomText = this.translationService.t('Do not poweroff the system while updating !...');

    this.CheckForUpdate();
  }

  ngOnDestroy(): void {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    if (typeof this.mytimer2 !== 'undefined') {
      this.mytimer2.unsubscribe();
      this.mytimer2 = undefined;
    }
  }

  private CheckForUpdate() {
    // this.statusText = 'Getting version number...';
    this.updateService.checkForUpdate(true).subscribe(data => {
      if (data.HaveUpdate === true) {
        this.topText = this.translationService.t('Update Available... Downloading Update !...');
        this.mytimer = interval(400).subscribe(() => {
          this.StartUpdate();
        });
      } else {
        this.topText = this.translationService.t('Could not start download,<br>check your internet connection or try again later !...');
      }
    }, error => {
      this.topText = this.translationService.t('Error communicating with Server !...');
    });
  }

  private StartUpdate() {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    this.progressDataPercentage = 0;
    this.displayProgress = true;

    this.updateService.updateApplication().subscribe(data => {
      if (data.status === 'OK') {
        this.mytimer = interval(600).subscribe(() => {
          this.progressupdatesystem();
        });
        this.mytimer2 = interval(1000).subscribe(() => {
          this.CheckUpdateReader();
        });
      } else {
        this.topText = this.translationService.t('Could not start download,<br>check your internet connection or try again later !...');
      }
    }, error => {
      this.topText = this.translationService.t('Error communicating with Server !...');
    });
  }

  private progressupdatesystem() {
    const val = this.progressDataPercentage;
    this.progressDataPercentage = val + 1;

    if (val === 100 || this.updateReady) {
      if (typeof this.mytimer !== 'undefined') {
        this.mytimer.unsubscribe();
        this.mytimer = undefined;
      }
      if (typeof this.mytimer2 !== 'undefined') {
        this.mytimer2.unsubscribe();
        this.mytimer2 = undefined;
      }
      if (this.updateReady === false) {
        this.displayProgress = false;
        this.topText = this.translationService.t('Error while downloading Update,<br>check your internet connection or try again later !...');
      } else {
        this.router.navigate(['/Dashboard']);
      }
    }
  }

  private CheckUpdateReader() {
    if (typeof this.mytimer2 !== 'undefined') {
      this.mytimer2.unsubscribe();
      this.mytimer2 = undefined;
    }
    // this.statusText = 'Getting version number...';

    this.updateService.getVersion().subscribe(data => {
      if (data.status === 'OK') {
        this.newVersion = data.version;
        this.configService.appversion.next(this.newVersion);
        if (this.appVersion !== this.newVersion) {
          // We are ready !
          // this.statusText = 'Got new version';
          this.updateReady = true;
        } else {
          // this.statusText = 'Got same version...';
          this.mytimer2 = interval(1000).subscribe(() => {
            this.CheckUpdateReader();
          });
        }
      } else {
        // this.statusText = 'Got a error returned!...';
        this.mytimer2 = interval(1500).subscribe(() => {
          this.CheckUpdateReader();
        });
      }
    }, error => {
      // this.statusText = 'Could not get version !';
      this.mytimer2 = interval(1000).subscribe(() => {
        this.CheckUpdateReader();
      });
    });
  }

}
