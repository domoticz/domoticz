import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FrontpageRoutingModule} from './frontpage-routing.module';
import {FrontpageComponent} from "./frontpage/frontpage.component";

@NgModule({
  imports: [
    CommonModule,
    /* Routing */
    FrontpageRoutingModule,
    /* Domoticz */
  ],
  declarations: [
    FrontpageComponent
  ],
  exports: [],
  providers: []
})
export class FrontpageModule {
}
