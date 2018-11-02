import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {ForecastComponent} from './forecast.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {ForecastModule} from '../forecast.module';

describe('ForecastComponent', () => {
  let component: ForecastComponent;
  let fixture: ComponentFixture<ForecastComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, ForecastModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ForecastComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
