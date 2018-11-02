import {Injectable, Inject} from '@angular/core';
import {PermissionService} from './permission.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Observable, throwError, of} from 'rxjs';
import {NotificationService} from './notification.service';

@Injectable()
export class ApiHelperService {

  constructor(
    private permissionService: PermissionService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService
  ) {
  }

  public checkViewerPermissions() {
    return this.checkPermissions('Viewer');
  }

  public checkUserPermissions() {
    return this.checkPermissions('User');
  }

  public checkAdminPermissions() {
    return this.checkPermissions('Admin');
  }

  // FIXME just use a boolean rather than Observable?
  private checkPermissions(value: string): Observable<void> {
    if (!this.permissionService.hasPermission(value)) {
      const message = this.translationService.t('You do not have permission to do that!');
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(message, 2500, true);
      return throwError(message);
    } else {
      return of(null);
    }
  }

}
