import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {CamRoutingModule} from './cam-routing.module';
import {FormsModule} from '@angular/forms';
import {CamComponent} from './cam/cam.component';
import {SharedModule} from '../../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    CamRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    CamComponent
  ],
  exports: [],
  providers: []
})
export class CamModule {
}
