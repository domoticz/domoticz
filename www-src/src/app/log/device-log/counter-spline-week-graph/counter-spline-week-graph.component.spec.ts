import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterSplineWeekGraphComponent } from './counter-spline-week-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterSplineWeekGraphComponent', () => {
  let component: CounterSplineWeekGraphComponent;
  let fixture: ComponentFixture<CounterSplineWeekGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterSplineWeekGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterSplineWeekGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
