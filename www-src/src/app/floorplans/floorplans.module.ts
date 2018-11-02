import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FloorplansdRoutingModule} from './floorplans-routing.module';
import {FloorplansComponent} from './floorplans/floorplans.component';

@NgModule({
  imports: [
    CommonModule,
    /* Routing */
    FloorplansdRoutingModule,
    /* Domoticz */
  ],
  declarations: [
    FloorplansComponent
  ],
  exports: [],
  providers: []
})
export class FloorplansModule {
}
