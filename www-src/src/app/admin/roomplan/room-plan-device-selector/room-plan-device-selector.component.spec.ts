import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlanDeviceSelectorComponent} from './room-plan-device-selector.component';
import {CommonTestModule} from "../../../_testing/common.test.module";
import {RoomPlanDeviceSelectorTableComponent} from "../room-plan-device-selector-table/room-plan-device-selector-table.component";

describe('RoomPlanDeviceSelectorComponent', () => {
  let component: RoomPlanDeviceSelectorComponent;
  let fixture: ComponentFixture<RoomPlanDeviceSelectorComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RoomPlanDeviceSelectorComponent, RoomPlanDeviceSelectorTableComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlanDeviceSelectorComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
