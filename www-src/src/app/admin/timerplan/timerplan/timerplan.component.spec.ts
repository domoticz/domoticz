import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {TimerplanComponent} from './timerplan.component';
import {CommonTestModule} from "../../../_testing/common.test.module";

describe('TimerplanComponent', () => {
  let component: TimerplanComponent;
  let fixture: ComponentFixture<TimerplanComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [TimerplanComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TimerplanComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
