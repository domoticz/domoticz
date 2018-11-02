import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterMonthGraphComponent } from './counter-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterMonthGraphComponent', () => {
  let component: CounterMonthGraphComponent;
  let fixture: ComponentFixture<CounterMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterMonthGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterMonthGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
