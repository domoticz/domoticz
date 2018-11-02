import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { SmartMonthGraphComponent } from './smart-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('SmartMonthGraphComponent', () => {
  let component: SmartMonthGraphComponent;
  let fixture: ComponentFixture<SmartMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [SmartMonthGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SmartMonthGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';
    component.switchtype = 1;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
