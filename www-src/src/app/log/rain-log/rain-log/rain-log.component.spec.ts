import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RainLogComponent } from './rain-log.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { RainDayGraphComponent } from '../rain-day-graph/rain-day-graph.component';
import { RainWeekGraphComponent } from '../rain-week-graph/rain-week-graph.component';
import { RainMonthGraphComponent } from '../rain-month-graph/rain-month-graph.component';
import { RainYearGraphComponent } from '../rain-year-graph/rain-year-graph.component';

describe('RainLogComponent', () => {
  let component: RainLogComponent;
  let fixture: ComponentFixture<RainLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RainLogComponent, RainDayGraphComponent, RainWeekGraphComponent, RainMonthGraphComponent, RainYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RainLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
