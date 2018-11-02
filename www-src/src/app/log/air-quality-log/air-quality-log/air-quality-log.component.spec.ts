import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AirQualityLogComponent} from './air-quality-log.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {AirQualityDayGraphComponent} from '../air-quality-day-graph/air-quality-day-graph.component';
import {AirQualityMonthGraphComponent} from '../air-quality-month-graph/air-quality-month-graph.component';
import {AirQualityYearGraphComponent} from '../air-quality-year-graph/air-quality-year-graph.component';

describe('AirQualityLogComponent', () => {
  let component: AirQualityLogComponent;
  let fixture: ComponentFixture<AirQualityLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [AirQualityLogComponent, AirQualityDayGraphComponent, AirQualityMonthGraphComponent, AirQualityYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AirQualityLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
