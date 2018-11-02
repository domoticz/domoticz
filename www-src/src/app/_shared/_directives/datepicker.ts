import { ConfigService } from '../_services/config.service';
import { Directive, ElementRef, Output, EventEmitter, Input, Optional } from '@angular/core';
import { NgModel, FormControl, NgControl } from '@angular/forms';

// FIXME manage with NPM+TypeScript
declare var $: any;

// FIXME replace with ng-bootstrap or any other angular lib
@Directive({
    selector: '[dzDatePicker]'
})
export class DatePickerDirective {

    // @Output() close = new EventEmitter<void>();

    constructor(private el: ElementRef,
        configService: ConfigService,
        ngControl: NgControl) {
        $(el.nativeElement).datepicker({
            dateFormat: configService.config.DateFormat,
            onClose: (val: string) => {
                ngControl.control.setValue(val);
                // this.close.emit();
            }
        });
    }

    // @Input()
    // set setDate(setDate: string) {
    //     $(this.el.nativeElement).datepicker('setDate', setDate);
    // }

    @Input()
    set maxDate(maxDate: any) {
        $(this.el.nativeElement).datepicker('option', 'maxDate', maxDate);
    }

    @Input()
    set minDate(minDate: any) {
        $(this.el.nativeElement).datepicker('option', 'minDate', minDate);
    }

}
