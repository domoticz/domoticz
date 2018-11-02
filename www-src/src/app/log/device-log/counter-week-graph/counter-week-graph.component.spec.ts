import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterWeekGraphComponent } from './counter-week-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterWeekGraphComponent', () => {
  let component: CounterWeekGraphComponent;
  let fixture: ComponentFixture<CounterWeekGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterWeekGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterWeekGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
