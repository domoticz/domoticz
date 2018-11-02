import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {TimerFormComponent} from './timer-form.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('TimerFormComponent', () => {
  let component: TimerFormComponent;
  let fixture: ComponentFixture<TimerFormComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TimerFormComponent);
    component = fixture.componentInstance;

    component.colorSettingsType = '';
    component.timerSettings = {
      active: true,
      timertype: 0,
      date: '',
      hour: 0,
      min: 0,
      randomness: false,
      command: 0,
      level: null,
      tvalue: '',
      days: 0x80,
      color: '', // Empty string, intentionally illegal JSON
      mday: 1,
      month: 1,
      occurence: 1,
      weekday: 0,
    };

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
