import { Directive, TemplateRef, ViewContainerRef } from '@angular/core';
import { PermissionService } from '../_services/permission.service';

@Directive({
    selector: '[dzHasLoginNoAdmin]'
})
export class HasLoginNoAdminDirective {

    private hasView = false;

    constructor(private templateRef: TemplateRef<any>,
        private viewContainer: ViewContainerRef,
        permissionService: PermissionService) {
        permissionService.getPermission().subscribe(permissions => {
            let bVisible: boolean = !permissions.hasPermission('Admin');
            if (bVisible) {
                bVisible = permissions.hasLogin(true);
            }
            const condition = bVisible;
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
