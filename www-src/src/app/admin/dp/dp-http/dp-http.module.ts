import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {DpHttpRoutingModule} from './dp-http-routing.module';
import {DpHttpComponent} from './dp-http.component';
import {SharedModule} from '../../../_shared/shared.module';
import {DpHttpService} from './dp-http.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DpHttpRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    DpHttpComponent,
  ],
  exports: [],
  providers: [
    DpHttpService
  ]
})
export class DpHttpModule {
}
