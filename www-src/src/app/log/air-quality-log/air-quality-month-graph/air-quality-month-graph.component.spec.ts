import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { AirQualityMonthGraphComponent } from './air-quality-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('AirQualityMonthGraphComponent', () => {
  let component: AirQualityMonthGraphComponent;
  let fixture: ComponentFixture<AirQualityMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ AirQualityMonthGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AirQualityMonthGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
