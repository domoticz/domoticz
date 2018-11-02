import { Component, OnInit, Inject } from '@angular/core';
import { LanguageService } from '../../_shared/_services/language.service';
import { mergeMap } from 'rxjs/operators';
import { LoginService } from '../../_shared/_services/login.service';
import { NotificationService } from '../../_shared/_services/notification.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { PermissionService } from '../../_shared/_services/permission.service';
import { Permissions } from 'src/app/_shared/_models/permissions';
import { Router } from '@angular/router';
import { ConfigService } from '../../_shared/_services/config.service';

@Component({
  selector: 'dz-login',
  templateUrl: './login.component.html',
  styleUrls: ['./login.component.css']
})
export class LoginComponent implements OnInit {

  canDisplay = false;

  // Form data
  username: string;
  password: string;
  rememberme = false;

  private failcounter = 0;

  constructor(
    private languageService: LanguageService,
    private loginService: LoginService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private permissionService: PermissionService,
    private router: Router,
    private configService: ConfigService
  ) { }

  ngOnInit() {
    this.languageService.getLanguage().pipe(
      mergeMap(lng => this.languageService.setLanguage(lng))
    ).subscribe(() => {
      // Allow to wait for i18n setup before rendering the view
      // There is other ways to do so but this one feels easy to understand
      this.canDisplay = true;
    }, error => {
      // Nothing
    });
  }

  DoLogin() {
    this.loginService.loginCheck(this.username, this.password, this.rememberme).subscribe(data => {
      if (data.status !== 'OK') {
        this.notificationService.HideNotify();
        this.failcounter += 1;
        if (this.failcounter > 3) {
          window.location.href = 'http://www.1112.net/lastpage.html';
          return;
        } else {
          this.notificationService.ShowNotify(this.translationService.t('Incorrect Username/Password!'), 2500, true);
        }
        return;
      }
      const permissionList: Permissions = new Permissions(0, true);
      if (data.user !== '') {
        permissionList.isloggedin = true;
      }
      permissionList.rights = Number(data.rights);
      this.permissionService.setPermission(permissionList);

      // $rootScope.GetGlobalConfig();
      this.configService.getConfigFromApi().subscribe(apiConfig => {
        this.configService.applyConfigFromApi(apiConfig);
        this.router.navigate(['/dashboard']);
      });
    }, error => {
      this.notificationService.HideNotify();
      this.failcounter += 1;
      if (this.failcounter > 3) {
        window.location.href = 'http://www.1112.net/lastpage.html';
        return;
      } else {
        this.notificationService.ShowNotify(this.translationService.t('Incorrect Username/Password!'), 2500, true);
      }
    });
  }

}
