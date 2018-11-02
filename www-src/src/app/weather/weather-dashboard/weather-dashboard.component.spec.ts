import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {WeatherDashboardComponent} from './weather-dashboard.component';
import {WeatherWidgetComponent} from '../weather-widget/weather-widget.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';

describe('WeatherDashboardComponent', () => {
  let component: WeatherDashboardComponent;
  let fixture: ComponentFixture<WeatherDashboardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [WeatherDashboardComponent, WeatherWidgetComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(WeatherDashboardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
