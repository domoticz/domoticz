import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {ScenesRoutingModule} from './scenes-routing.module';
import {ScenesDashboardComponent} from './scenes-dashboard/scenes-dashboard.component';
import {SceneWidgetComponent} from './scene-widget/scene-widget.component';
import {EditSceneComponent} from './edit-scene/edit-scene.component';
import {EditSceneActivationTableComponent} from './edit-scene-activation-table/edit-scene-activation-table.component';
import {EditSceneDevicesTableComponent} from './edit-scene-devices-table/edit-scene-devices-table.component';
import {SceneTimersComponent} from './scene-timers/scene-timers.component';
import {SceneLogComponent} from './scene-log/scene-log.component';
import {SharedModule} from '../_shared/shared.module';
import {SceneTimersService} from './scene-timers.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    ScenesRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    ScenesDashboardComponent,
    SceneWidgetComponent,
    EditSceneComponent,
    EditSceneActivationTableComponent,
    EditSceneDevicesTableComponent,
    SceneTimersComponent,
    SceneLogComponent
  ],
  exports: [],
  providers: [
    SceneTimersService
  ]
})
export class ScenesModule {
}
