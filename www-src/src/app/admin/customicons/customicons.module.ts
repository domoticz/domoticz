import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {CustomiconsRoutingModule} from './customicons-routing.module';
import {CustomiconsComponent} from './customicons.component';
import {SharedModule} from '../../_shared/shared.module';
import {IconService} from './icon.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    CustomiconsRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    CustomiconsComponent
  ],
  exports: [],
  providers: [
    IconService
  ]
})
export class CustomiconsModule {
}
