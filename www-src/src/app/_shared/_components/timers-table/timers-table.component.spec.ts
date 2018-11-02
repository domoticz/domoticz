import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {TimersTableComponent} from './timers-table.component';
import {CommonTestModule} from '../../../_testing/common.test.module';

describe('TimersTableComponent', () => {
  let component: TimersTableComponent;
  let fixture: ComponentFixture<TimersTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TimersTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
