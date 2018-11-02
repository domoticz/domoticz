import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { NotificationsTableComponent } from './notifications-table.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import {DeviceNotificationsModule} from "../device-notifications.module";

describe('NotificationsTableComponent', () => {
  let component: NotificationsTableComponent;
  let fixture: ComponentFixture<NotificationsTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [  ],
      imports: [
        CommonTestModule,
        DeviceNotificationsModule
      ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(NotificationsTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
