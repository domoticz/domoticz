import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterSplineDayGraphComponent } from './counter-spline-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterSplineDayGraphComponent', () => {
  let component: CounterSplineDayGraphComponent;
  let fixture: ComponentFixture<CounterSplineDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterSplineDayGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterSplineDayGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
