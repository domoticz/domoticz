import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {DevicesRoutingModule} from './devices-routing.module';
import {FormsModule} from '@angular/forms';
import {DevicesComponent} from './devices/devices.component';
import {DeviceFilterByUsageComponent} from './device-filter-by-usage/device-filter-by-usage.component';
import {DevicesFiltersComponent} from './devices-filters/devices-filters.component';
import {DevicesTableComponent} from './devices-table/devices-table.component';
import {DeviceAddModalComponent} from './device-add-modal/device-add-modal.component';
import {DeviceRenameModalComponent} from './device-rename-modal/device-rename-modal.component';
import {SharedModule} from '../../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DevicesRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    DevicesComponent,
    DeviceFilterByUsageComponent,
    DevicesFiltersComponent,
    DevicesTableComponent,
    DeviceAddModalComponent,
    DeviceRenameModalComponent
  ],
  exports: [],
  providers: []
})
export class DevicesModule {
}
