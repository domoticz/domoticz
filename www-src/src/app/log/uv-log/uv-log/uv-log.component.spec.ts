import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { UvLogComponent } from './uv-log.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { UvDayGraphComponent } from '../uv-day-graph/uv-day-graph.component';
import { UvYearGraphComponent } from '../uv-year-graph/uv-year-graph.component';
import { UvMonthGraphComponent } from '../uv-month-graph/uv-month-graph.component';

describe('UvLogComponent', () => {
  let component: UvLogComponent;
  let fixture: ComponentFixture<UvLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ UvLogComponent, UvDayGraphComponent, UvMonthGraphComponent, UvYearGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UvLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
