import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlanDeviceSelectorModalComponent} from './room-plan-device-selector-modal.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {RoomPlanDeviceSelectorTableComponent} from '../room-plan-device-selector-table/room-plan-device-selector-table.component';
import {RoomPlanDeviceSelectorComponent} from '../room-plan-device-selector/room-plan-device-selector.component';

describe('RoomPlanDeviceSelectorModalComponent', () => {
  let component: RoomPlanDeviceSelectorModalComponent;
  let fixture: ComponentFixture<RoomPlanDeviceSelectorModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RoomPlanDeviceSelectorModalComponent, RoomPlanDeviceSelectorTableComponent, RoomPlanDeviceSelectorComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlanDeviceSelectorModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
