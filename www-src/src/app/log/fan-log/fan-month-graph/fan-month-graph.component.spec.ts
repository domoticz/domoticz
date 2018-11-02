import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { FanMonthGraphComponent } from './fan-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('FanMonthGraphComponent', () => {
  let component: FanMonthGraphComponent;
  let fixture: ComponentFixture<FanMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ FanMonthGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FanMonthGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
