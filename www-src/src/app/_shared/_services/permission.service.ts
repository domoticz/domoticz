import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { Permissions } from '../_models/permissions';

// FIXME rename properly stuff in this class
@Injectable()
export class PermissionService {

  private defaultPermission: Permissions = new Permissions(-1, false);

  // Replace the event permissionsChanged
  private permissionList: BehaviorSubject<Permissions> = new BehaviorSubject<Permissions>(this.defaultPermission);

  constructor() { }

  getPermission(): Observable<Permissions> {
    return this.permissionList;
  }

  setPermission(permissions: Permissions): void {
    this.permissionList.next(permissions);
  }

  hasPermission(permission: string): boolean {
    return this.permissionList.getValue().hasPermission(permission);
  }

  hasLogin(isloggedin: boolean): boolean {
    return this.permissionList.getValue().hasLogin(isloggedin);
  }

  isAuthenticated(): boolean {
    return this.permissionList.getValue().isAuthenticated();
  }

  getRights(): number {
    return this.permissionList.getValue().rights;
  }

}
