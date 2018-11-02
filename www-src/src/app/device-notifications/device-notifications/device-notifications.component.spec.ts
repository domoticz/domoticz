import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DeviceNotificationsComponent} from './device-notifications.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DeviceNotificationsModule} from "../device-notifications.module";

describe('DeviceNotificationsComponent', () => {
  let component: DeviceNotificationsComponent;
  let fixture: ComponentFixture<DeviceNotificationsComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [
        CommonTestModule, FormsModule,
        DeviceNotificationsModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceNotificationsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
