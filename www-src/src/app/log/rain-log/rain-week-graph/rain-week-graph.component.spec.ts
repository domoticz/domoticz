import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RainWeekGraphComponent } from './rain-week-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('RainWeekGraphComponent', () => {
  let component: RainWeekGraphComponent;
  let fixture: ComponentFixture<RainWeekGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RainWeekGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RainWeekGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
