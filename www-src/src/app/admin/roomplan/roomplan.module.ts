import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {RoomplanRoutingModule} from './roomplan-routing.module';
import {RoomplanComponent} from './roomplan/roomplan.component';
import {RoomPlansTableComponent} from './room-plans-table/room-plans-table.component';
import {RoomPlanEditModalComponent} from './room-plan-edit-modal/room-plan-edit-modal.component';
import {RoomPlanDevicesTableComponent} from './room-plan-devices-table/room-plan-devices-table.component';
import {RoomPlanAddModalComponent} from './room-plan-add-modal/room-plan-add-modal.component';
import {RoomPlanDeviceSelectorModalComponent} from './room-plan-device-selector-modal/room-plan-device-selector-modal.component';
import {RoomPlanDeviceSelectorComponent} from './room-plan-device-selector/room-plan-device-selector.component';
import {RoomPlanDeviceSelectorTableComponent} from './room-plan-device-selector-table/room-plan-device-selector-table.component';
import {SharedModule} from '../../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    RoomplanRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    RoomplanComponent,
    RoomPlansTableComponent,
    RoomPlanEditModalComponent,
    RoomPlanDevicesTableComponent,
    RoomPlanAddModalComponent,
    RoomPlanDeviceSelectorModalComponent,
    RoomPlanDeviceSelectorComponent,
    RoomPlanDeviceSelectorTableComponent
  ],
  exports: [],
  providers: []
})
export class RoomplanModule {
}
