import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {AboutComponent} from './about/about.component';
import {AboutRoutingModule} from './about-routing.module';
import {SharedModule} from '../../_shared/shared.module';
import {AboutService} from './about.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    AboutRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    AboutComponent
  ],
  exports: [],
  providers: [
    AboutService
  ]
})
export class AboutModule {
}
