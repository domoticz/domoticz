import { Directive, Input, TemplateRef, ViewContainerRef } from '@angular/core';
import { PermissionService } from '../_services/permission.service';

@Directive({
    selector: '[dzHasLogin]'
})
export class HasLoginDirective {

    private hasView = false;

    constructor(private templateRef: TemplateRef<any>,
        private viewContainer: ViewContainerRef,
        private permissionService: PermissionService) {
    }

    // FIXME check if we really use false sometimes or always true?
    @Input()
    set dzHasLogin(hasLogin: boolean) {
        this.permissionService.getPermission().subscribe(permissions => {
            const condition = permissions.hasLogin(hasLogin);
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
