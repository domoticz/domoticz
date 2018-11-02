import {RouterModule, Routes} from '@angular/router';
import {UvLogComponent} from './uv-log/uv-log.component';
import {OfflineGuard} from '../../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../../_shared/_guards/authentication.guard';
import {NgModule} from '@angular/core';

const routes: Routes = [
  {
    path: '',
    component: UvLogComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class UvLogRoutingModule {
}
