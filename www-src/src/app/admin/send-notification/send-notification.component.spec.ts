import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {SendNotificationComponent} from './send-notification.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {SendNotificationModule} from "./send-notification.module";

describe('SendNotificationComponent', () => {
  let component: SendNotificationComponent;
  let fixture: ComponentFixture<SendNotificationComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, SendNotificationModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SendNotificationComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
