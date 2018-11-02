import {Injectable} from '@angular/core';
import {ActivatedRouteSnapshot, CanActivate, Router, RouterStateSnapshot} from '@angular/router';
import {PermissionService} from '../_services/permission.service';
import {Observable} from 'rxjs';
import {map} from 'rxjs/operators';

@Injectable()
export class AuthenticationGuard implements CanActivate {

  constructor(private permissionService: PermissionService,
              private router: Router) {
  }

  canActivate(next: ActivatedRouteSnapshot, state: RouterStateSnapshot): Observable<boolean> {
    return this.permissionService.getPermission().pipe(
      map(permission => {
        if (!permission.isAuthenticated()) {
          this.router.navigate(['/Login']);
          return false;
        } else {
          const routePermission = next.data.permission;
          if (routePermission && !permission.hasPermission(routePermission)) {
            this.router.navigate(['/Dashboard']);
            return false;
          } else {
            return true;
          }
        }
      })
    );
  }

}
