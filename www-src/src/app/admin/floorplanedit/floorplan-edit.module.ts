import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {FloorplanEditRoutingModule} from './floorplan-edit-routing.module';
import {FloorplanEditComponent} from './floorplan-edit/floorplan-edit.component';
import {SharedModule} from '../../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    FloorplanEditRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    FloorplanEditComponent
  ],
  exports: [],
  providers: []
})
export class FloorplanEditModule {
}
