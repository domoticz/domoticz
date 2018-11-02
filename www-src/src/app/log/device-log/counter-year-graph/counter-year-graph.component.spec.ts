import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CounterYearGraphComponent } from './counter-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CounterYearGraphComponent', () => {
  let component: CounterYearGraphComponent;
  let fixture: ComponentFixture<CounterYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CounterYearGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CounterYearGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
