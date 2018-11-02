import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {SetpointPopupComponent} from './setpoint-popup.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('SetpointPopupComponent', () => {
  let component: SetpointPopupComponent;
  let fixture: ComponentFixture<SetpointPopupComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SetpointPopupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
