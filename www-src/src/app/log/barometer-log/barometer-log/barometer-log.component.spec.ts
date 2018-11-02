import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { BarometerLogComponent } from './barometer-log.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { BarometerDayGraphComponent } from '../barometer-day-graph/barometer-day-graph.component';
import { BarometerMonthGraphComponent } from '../barometer-month-graph/barometer-month-graph.component';
import { BarometerYearGraphComponent } from '../barometer-year-graph/barometer-year-graph.component';

describe('BarometerLogComponent', () => {
  let component: BarometerLogComponent;
  let fixture: ComponentFixture<BarometerLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [BarometerLogComponent, BarometerDayGraphComponent, BarometerMonthGraphComponent, BarometerYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(BarometerLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
