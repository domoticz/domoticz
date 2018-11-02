import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterDayGraphComponent } from './counter-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterDayGraphComponent', () => {
  let component: CounterDayGraphComponent;
  let fixture: ComponentFixture<CounterDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterDayGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterDayGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
