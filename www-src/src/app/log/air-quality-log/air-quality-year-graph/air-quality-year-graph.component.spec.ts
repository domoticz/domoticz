import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { AirQualityYearGraphComponent } from './air-quality-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('AirQualityYearGraphComponent', () => {
  let component: AirQualityYearGraphComponent;
  let fixture: ComponentFixture<AirQualityYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ AirQualityYearGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AirQualityYearGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
