import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterSplineMonthGraphComponent } from './counter-spline-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterSplineMonthGraphComponent', () => {
  let component: CounterSplineMonthGraphComponent;
  let fixture: ComponentFixture<CounterSplineMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterSplineMonthGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterSplineMonthGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
