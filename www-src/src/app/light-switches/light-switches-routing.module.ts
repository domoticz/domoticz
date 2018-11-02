import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {LightSwitchesDashboardComponent} from './light-switches-dashboard/light-switches-dashboard.component';

const routes: Routes = [
  {
    path: '',
    component: LightSwitchesDashboardComponent
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class LightSwitchesRoutingModule {
}
