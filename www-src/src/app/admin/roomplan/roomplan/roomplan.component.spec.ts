import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomplanComponent} from './roomplan.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {RoomPlanAddModalComponent} from '../room-plan-add-modal/room-plan-add-modal.component';
import {RoomPlanDeviceSelectorModalComponent} from '../room-plan-device-selector-modal/room-plan-device-selector-modal.component';
import {RoomPlansTableComponent} from '../room-plans-table/room-plans-table.component';
import {RoomPlanDevicesTableComponent} from '../room-plan-devices-table/room-plan-devices-table.component';
import {RoomPlanEditModalComponent} from '../room-plan-edit-modal/room-plan-edit-modal.component';
import {RoomPlanDeviceSelectorComponent} from '../room-plan-device-selector/room-plan-device-selector.component';
import {RoomPlanDeviceSelectorTableComponent} from '../room-plan-device-selector-table/room-plan-device-selector-table.component';

describe('RoomplanComponent', () => {
  let component: RoomplanComponent;
  let fixture: ComponentFixture<RoomplanComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        RoomplanComponent,
        RoomPlansTableComponent,
        RoomPlanDevicesTableComponent,
        RoomPlanAddModalComponent,
        RoomPlanDeviceSelectorModalComponent,
        RoomPlanEditModalComponent,
        RoomPlanDeviceSelectorComponent,
        RoomPlanDeviceSelectorTableComponent
      ],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomplanComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
