import { ConfigService } from '../_services/config.service';
import { PermissionService } from '../_services/permission.service';
import { Directive, ElementRef, Output, EventEmitter } from '@angular/core';

// FIXME manage with NPM+TypeScript
declare var $: any;

// FIXME replace with ng-bootstrap or any other angular lib
@Directive({
    selector: '[dzDragDrop]'
})
export class DragDropDirective {

    @Output()
    drag: EventEmitter<void> = new EventEmitter<void>();

    @Output()
    drop: EventEmitter<void> = new EventEmitter<void>();

    constructor(
        configService: ConfigService,
        permissionService: PermissionService,
        el: ElementRef
    ) {

        const elt = el.nativeElement;

        if (configService.config.AllowWidgetOrdering
            && configService.globals.ismobileint === false
            && permissionService.hasPermission('Admin')) {

            const _this = this;

            $(elt).draggable({
                drag: () => {
                    _this.drag.emit(undefined);
                    elt.style['z-index'] = 2;
                },
                revert: true
            });

            $(elt).droppable({
                drop: () => {
                    _this.drop.emit(undefined);
                }
            });
        }
    }

}
