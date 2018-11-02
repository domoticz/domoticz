import {Component, Inject, OnInit} from '@angular/core';
import {GlobalConfig} from '../_shared/_models/config';
import {ConfigService} from '../_shared/_services/config.service';
import {Router} from '@angular/router';
import {UpdateService} from '../_shared/_services/update.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ApiService} from '../_shared/_services/api.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-header',
  templateUrl: './header.component.html',
  styleUrls: ['./header.component.css']
})
export class HeaderComponent implements OnInit {

  config: GlobalConfig;
  appversion: string;

  templates: Array<{ file: string, name: string }> = [];

  constructor(
    private configService: ConfigService,
    private router: Router,
    private updateService: UpdateService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private apiService: ApiService
  ) {
  }

  ngOnInit() {
    this.config = this.configService.config;
    this.configService.appversion.subscribe(version => {
      this.appversion = version;
    });
    this.templates = this.configService.templates.map(item => {
      const name = item.charAt(0).toUpperCase() + item.slice(1);
      return {file: item, name: name};
    });
  }

  onAppversionClick() {
    this.configService.globals.historytype = 1;
    this.router.navigate(['/History']);
  }

  CheckForUpdate(showdialog: boolean) {
    this.updateService.checkForUpdate(true).subscribe(data => {
      if (data.HaveUpdate === true) {
        if (showdialog) {
          bootbox.confirm(this.translationService.t('Update Available !... Update Now?') + '<br>' +
            this.translationService.t('Version') + ' #' + data.Revision, (result) => {
            if (result === true) {
              if (data.SystemName === 'windows') {
                this.router.navigateByUrl(data.DomoticzUpdateURL);
              } else {
                this.router.navigate(['/Update']);
              }
            }
          });
        } else {
          this.configService.globals.historytype = 2;
          this.updateService.ShowUpdateNotification(data.Revision, data.SystemName, data.DomoticzUpdateURL);
        }
        return true;
      } else {
        if (showdialog) {
          bootbox.alert(this.translationService.t('No Update Available !...'));
        }
      }
    }, error => {
      if (showdialog) {
        bootbox.alert(this.translationService.t('Error communicating to server!'));
      }
    });
  }

  Restart() {
    bootbox.confirm(this.translationService.t('Are you sure to Restart the system?'), (result) => {
      if (result === true) {
        this.apiService.callApi('command', {param: 'system_reboot'}).subscribe(() => {
          // Nothing to do
        }, error => {
          // Nothing to do
        });
        bootbox.alert(this.translationService.t('Restarting System (This could take some time...)'));
      }
    });
  }

  Shutdown() {
    bootbox.confirm(this.translationService.t('Are you sure to Shutdown the system?'), (result) => {
      if (result === true) {
        this.apiService.callApi('command', {param: 'system_shutdown'}).subscribe(() => {
          // Nothing to do
        }, error => {
          // Nothing to do
        });
        bootbox.alert(this.translationService.t('The system is being Shutdown (This could take some time...)'));
      }
    });
  }

}
