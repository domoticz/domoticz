import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {LightEditComponent} from './light-edit/light-edit.component';

const routes: Routes = [
  {
    path: '',
    component: LightEditComponent
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class LightEditRoutingModule {
}
