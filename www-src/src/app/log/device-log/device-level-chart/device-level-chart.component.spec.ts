import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceLevelChartComponent } from './device-level-chart.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('DeviceLevelChartComponent', () => {
  let component: DeviceLevelChartComponent;
  let fixture: ComponentFixture<DeviceLevelChartComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DeviceLevelChartComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceLevelChartComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
