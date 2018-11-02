import {Injectable} from '@angular/core';
import {ActivatedRouteSnapshot, CanActivate, Router, RouterStateSnapshot} from '@angular/router';
import {ConfigService} from "../_services/config.service";

@Injectable()
export class OfflineGuard implements CanActivate {

  constructor(private configService: ConfigService,
              private router: Router) {
  }

  canActivate(next: ActivatedRouteSnapshot, state: RouterStateSnapshot): boolean {
    if (!this.configService.online) {
      this.router.navigate(['/Offline']);
    }
    return this.configService.online;
  }

}
