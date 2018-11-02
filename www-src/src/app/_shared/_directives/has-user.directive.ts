import { Directive, ViewContainerRef, TemplateRef } from '@angular/core';
import { PermissionService } from '../_services/permission.service';

@Directive({
    selector: '[dzHasUser]'
})
export class HasUserDirective {

    private hasView = false;

    constructor(private templateRef: TemplateRef<any>,
        private viewContainer: ViewContainerRef,
        permissionService: PermissionService) {
        permissionService.getPermission().subscribe(permissions => {
            const condition = permissions.isAuthenticated();
            if (condition && !this.hasView) {
                this.viewContainer.createEmbeddedView(this.templateRef);
                this.hasView = true;
            } else if (!condition && this.hasView) {
                this.viewContainer.clear();
                this.hasView = false;
            }
        });
    }

}
