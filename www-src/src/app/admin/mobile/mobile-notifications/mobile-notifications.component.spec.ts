import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { MobileNotificationsComponent } from './mobile-notifications.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';
import {MobileModule} from "../mobile.module";

describe('MobileNotificationsComponent', () => {
  let component: MobileNotificationsComponent;
  let fixture: ComponentFixture<MobileNotificationsComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, MobileModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(MobileNotificationsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
