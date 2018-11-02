import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../../_shared/_guards/authentication.guard';
import {UserVariablesComponent} from './user-variables.component';

const routes: Routes = [
  {
    path: '',
    component: UserVariablesComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class UserVariablesRoutingModule {
}
