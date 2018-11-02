import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { SmartWeekGraphComponent } from './smart-week-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('SmartWeekGraphComponent', () => {
  let component: SmartWeekGraphComponent;
  let fixture: ComponentFixture<SmartWeekGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [SmartWeekGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SmartWeekGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';
    component.switchtype = 1;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
