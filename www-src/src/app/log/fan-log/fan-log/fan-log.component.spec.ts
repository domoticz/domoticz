import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {FanLogComponent} from './fan-log.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FanDayGraphComponent} from '../fan-day-graph/fan-day-graph.component';
import {FanMonthGraphComponent} from '../fan-month-graph/fan-month-graph.component';
import {FanYearGraphComponent} from '../fan-year-graph/fan-year-graph.component';

describe('FanLogComponent', () => {
  let component: FanLogComponent;
  let fixture: ComponentFixture<FanLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [FanLogComponent, FanDayGraphComponent, FanMonthGraphComponent, FanYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FanLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
