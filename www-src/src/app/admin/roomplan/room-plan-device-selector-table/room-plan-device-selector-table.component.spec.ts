import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlanDeviceSelectorTableComponent} from './room-plan-device-selector-table.component';
import {CommonTestModule} from "../../../_testing/common.test.module";

describe('RoomPlanDeviceSelectorTableComponent', () => {
  let component: RoomPlanDeviceSelectorTableComponent;
  let fixture: ComponentFixture<RoomPlanDeviceSelectorTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RoomPlanDeviceSelectorTableComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlanDeviceSelectorTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
