import { ConfigService } from '../_services/config.service';
import { Directive, ElementRef } from '@angular/core';

// FIXME manage with NPM+TypeScript
declare var $: any;

// FIXME replace with ng-bootstrap or any other angular lib
@Directive({
    selector: '[dzDateTimePicker]'
})
export class DateTimePickerDirective {

    constructor(el: ElementRef,
        configService: ConfigService) {
        $(el.nativeElement).datetimepicker({
            dateFormat: configService.config.DateFormat,
        });
    }

}
