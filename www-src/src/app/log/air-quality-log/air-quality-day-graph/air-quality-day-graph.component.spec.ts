import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { AirQualityDayGraphComponent } from './air-quality-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('AirQualityDayGraphComponent', () => {
  let component: AirQualityDayGraphComponent;
  let fixture: ComponentFixture<AirQualityDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ AirQualityDayGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AirQualityDayGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
