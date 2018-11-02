import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {DpFibaroRoutingModule} from './dp-fibaro-routing.module';
import {DpFibaroComponent} from './dp-fibaro.component';
import {SharedModule} from '../../../_shared/shared.module';
import {FibaroService} from './fibaro.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DpFibaroRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    DpFibaroComponent,
  ],
  exports: [],
  providers: [
    FibaroService
  ]
})
export class DpFibaroModule {
}
