import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {TemperatureForecastComponent} from './temperature-forecast.component';
import {CommonTestModule} from '../../_testing/common.test.module';

describe('TemperatureForecastComponent', () => {
  let component: TemperatureForecastComponent;
  let fixture: ComponentFixture<TemperatureForecastComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [TemperatureForecastComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TemperatureForecastComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
