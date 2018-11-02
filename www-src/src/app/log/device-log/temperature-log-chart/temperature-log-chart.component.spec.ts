import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { TemperatureLogChartComponent } from './temperature-log-chart.component';
import { CommonTestModule } from '../../../_testing/common.test.module';

describe('TemperatureLogChartComponent', () => {
  let component: TemperatureLogChartComponent;
  let fixture: ComponentFixture<TemperatureLogChartComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [TemperatureLogChartComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TemperatureLogChartComponent);
    component = fixture.componentInstance;

    component.deviceIdx = '157';
    component.deviceType = 'Temp + Humidity';
    component.range = 'day';

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
