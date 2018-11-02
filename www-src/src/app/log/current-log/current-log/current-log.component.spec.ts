import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CurrentLogComponent} from './current-log.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {CurrentDayGraphComponent} from '../current-day-graph/current-day-graph.component';
import {CurrentMonthGraphComponent} from '../current-month-graph/current-month-graph.component';
import {CurrentYearGraphComponent} from '../current-year-graph/current-year-graph.component';

describe('CurrentLogComponent', () => {
  let component: CurrentLogComponent;
  let fixture: ComponentFixture<CurrentLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [CurrentLogComponent, CurrentDayGraphComponent, CurrentMonthGraphComponent, CurrentYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CurrentLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
