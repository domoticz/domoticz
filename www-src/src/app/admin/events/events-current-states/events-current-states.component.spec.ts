import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {EventsCurrentStatesComponent} from './events-current-states.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {EventsModule} from "../events.module";

describe('EventsCurrentStatesComponent', () => {
  let component: EventsCurrentStatesComponent;
  let fixture: ComponentFixture<EventsCurrentStatesComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, EventsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(EventsCurrentStatesComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
