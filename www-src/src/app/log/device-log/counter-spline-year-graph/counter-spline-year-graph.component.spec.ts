import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterSplineYearGraphComponent } from './counter-spline-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterSplineYearGraphComponent', () => {
  let component: CounterSplineYearGraphComponent;
  let fixture: ComponentFixture<CounterSplineYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterSplineYearGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterSplineYearGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
