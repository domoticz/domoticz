import { Component, OnInit } from '@angular/core';
import { Permissions } from '../../_shared/_models/permissions';
import { PermissionService } from '../../_shared/_services/permission.service';
import { LoginService } from '../../_shared/_services/login.service';
import { mergeMap } from 'rxjs/operators';
import { Auth } from 'src/app/_shared/_models/auth';
import { Router } from '@angular/router';
import { HttpErrorResponse } from '@angular/common/http';

@Component({
  selector: 'dz-logout',
  templateUrl: './logout.component.html',
  styleUrls: ['./logout.component.css']
})
export class LogoutComponent implements OnInit {

  basicAuth = false;

  constructor(
    private permissionService: PermissionService,
    private loginService: LoginService,
    private router: Router
  ) { }

  ngOnInit() {
    const permissionList: Permissions = new Permissions(-1, true);
    this.permissionService.setPermission(permissionList);
    this.loginService.logout()
      .pipe(
        mergeMap(() => this.loginService.getAuth())
      ).subscribe(data => {
        if (data.status === 'OK') {
          if (data.user !== '') {
            permissionList.isloggedin = true;
          }
          permissionList.rights = data.rights;
        }
        this.permissionService.setPermission(permissionList);
        this.router.navigate(['/dashboard']);
      }, (error: HttpErrorResponse) => {
        const authenticate = error.headers.get('WWW-Authenticate');
        if (authenticate && (authenticate.indexOf('Basic') > -1)) {
          // When Basic authentication is used, user should close window after logout
          this.basicAuth = true;
          return;
        }
        this.router.navigate(['/dashboard']);
      });
  }

}
