import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {CustomRoutingModule} from './custom-routing.module';
import {CustomComponent} from './custom/custom.component';

@NgModule({
  imports: [
    CommonModule,
    /* Routing */
    CustomRoutingModule,
    /* Domoticz */
  ],
  declarations: [
    CustomComponent
  ],
  exports: [],
  providers: []
})
export class CustomModule {
}
