import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DeviceTimersComponent} from './device-timers.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DeviceTimersModule} from '../device-timers.module';

describe('DeviceTimersComponent', () => {
  let component: DeviceTimersComponent;
  let fixture: ComponentFixture<DeviceTimersComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, DeviceTimersModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceTimersComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
