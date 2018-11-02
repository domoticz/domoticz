import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {ScenesDashboardComponent} from './scenes-dashboard/scenes-dashboard.component';
import {EditSceneComponent} from './edit-scene/edit-scene.component';
import {OfflineGuard} from '../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../_shared/_guards/authentication.guard';
import {SceneLogComponent} from './scene-log/scene-log.component';
import {SceneTimersComponent} from './scene-timers/scene-timers.component';

const routes: Routes = [
  {
    path: ':idx/Edit',
    component: EditSceneComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: ':idx/Log',
    component: SceneLogComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: ':idx/Timers',
    component: SceneTimersComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: '',
    component: ScenesDashboardComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class ScenesRoutingModule {
}
