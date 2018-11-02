import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { BarometerMonthGraphComponent } from './barometer-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('BarometerMonthGraphComponent', () => {
  let component: BarometerMonthGraphComponent;
  let fixture: ComponentFixture<BarometerMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ BarometerMonthGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(BarometerMonthGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
