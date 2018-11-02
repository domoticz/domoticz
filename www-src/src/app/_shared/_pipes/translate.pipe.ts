import {Pipe, PipeTransform} from '@angular/core';
import {I18NextPipe} from 'angular-i18next';

/**
 * Alias for i18next pipe.
 */
@Pipe({
  name: 'translate'
})
export class TranslatePipe extends I18NextPipe implements PipeTransform {

  transform(value: any, args?: any): any {
    return super.transform(value as string);
  }

}
