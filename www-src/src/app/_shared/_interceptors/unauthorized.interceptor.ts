import { PermissionService } from '../_services/permission.service';
import { HttpInterceptor, HttpRequest, HttpHandler, HttpEvent } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { catchError } from 'rxjs/operators';
import { Permissions } from '../_models/permissions';
import { Router } from '@angular/router';

@Injectable()
export class UnauthorizedInterceptor implements HttpInterceptor {

    constructor(private permissionService: PermissionService,
        private router: Router) { }

    intercept(request: HttpRequest<any>, next: HttpHandler): Observable<HttpEvent<any>> {
        const response = next.handle(request);
        return response.pipe(
            catchError(error => {
                if (error.status === 401) {
                    console.log('UnauthorizedInterceptor');
                    this.permissionService.setPermission(new Permissions(-1, false));
                    this.router.navigate(['/Login']);
                }
                throw error;
            }),
        );
    }
}
