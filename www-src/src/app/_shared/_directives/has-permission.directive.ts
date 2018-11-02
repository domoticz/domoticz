import { Directive, Input, ViewContainerRef, TemplateRef } from '@angular/core';
import { PermissionService } from '../_services/permission.service';

@Directive({
    selector: '[dzHasPermission]'
})
export class HasPermissionDirective {

    private hasView = false;

    constructor(private templateRef: TemplateRef<any>,
        private viewContainer: ViewContainerRef,
        private permissionService: PermissionService) {
    }

    @Input()
    set dzHasPermission(permission: string) {
        this.permissionService.getPermission().subscribe(permissions => {
            let value = permission.trim();
            const notPermissionFlag = value[0] === '!';
            if (notPermissionFlag) {
                value = value.slice(1).trim();
            }
            const hasPermission = permissions.hasPermission(value);
            const condition = hasPermission && !notPermissionFlag || !hasPermission && notPermissionFlag;
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
