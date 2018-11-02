import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RainMonthGraphComponent } from './rain-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('RainMonthGraphComponent', () => {
  let component: RainMonthGraphComponent;
  let fixture: ComponentFixture<RainMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RainMonthGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RainMonthGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
