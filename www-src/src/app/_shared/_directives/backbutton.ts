import { Directive, HostListener } from '@angular/core';
import { Location } from '@angular/common';

@Directive({
    selector: '[dzBackButton]'
})
export class BackButtonDirective {

    constructor(private location: Location) {
    }

    @HostListener('click')
    onClick() {
        this.location.back();
    }

}
