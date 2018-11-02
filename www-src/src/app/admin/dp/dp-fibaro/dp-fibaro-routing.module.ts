import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {DpFibaroComponent} from './dp-fibaro.component';
import {AuthenticationGuard} from '../../../_shared/_guards/authentication.guard';
import {OfflineGuard} from '../../../_shared/_guards/offline.guard';

const routes: Routes = [
  {
    path: '',
    component: DpFibaroComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class DpFibaroRoutingModule {
}
