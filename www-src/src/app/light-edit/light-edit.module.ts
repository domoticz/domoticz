import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {LightEditRoutingModule} from './light-edit-routing.module';
import {FormsModule} from '@angular/forms';
import {LightEditComponent} from './light-edit/light-edit.component';
import {DeviceIconSelectComponent} from './device-icon-select/device-icon-select.component';
import {DeviceColorSettingsComponent} from './device-color-settings/device-color-settings.component';
import {DeviceLevelActionsTableComponent} from './device-level-actions-table/device-level-actions-table.component';
import {DeviceEditSelectorActionModalComponent} from './device-edit-selector-action-modal/device-edit-selector-action-modal.component';
import {DeviceSubdevicesEditorComponent} from './device-subdevices-editor/device-subdevices-editor.component';
import {DeviceLevelNamesTableComponent} from './device-level-names-table/device-level-names-table.component';
import {DeviceLevelRenameModalComponent} from './device-level-rename-modal/device-level-rename-modal.component';
import {SharedModule} from "../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    LightEditRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    LightEditComponent,
    DeviceIconSelectComponent,
    DeviceColorSettingsComponent,
    DeviceLevelActionsTableComponent,
    DeviceEditSelectorActionModalComponent,
    DeviceSubdevicesEditorComponent,
    DeviceLevelNamesTableComponent,
    DeviceLevelRenameModalComponent
  ],
  exports: [],
  providers: []
})
export class LightEditModule {
}
