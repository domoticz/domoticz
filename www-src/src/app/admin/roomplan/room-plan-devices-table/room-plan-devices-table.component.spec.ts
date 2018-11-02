import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlanDevicesTableComponent} from './room-plan-devices-table.component';
import {CommonTestModule} from '../../../_testing/common.test.module';

describe('RoomPlanDevicesTableComponent', () => {
  let component: RoomPlanDevicesTableComponent;
  let fixture: ComponentFixture<RoomPlanDevicesTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RoomPlanDevicesTableComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlanDevicesTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
