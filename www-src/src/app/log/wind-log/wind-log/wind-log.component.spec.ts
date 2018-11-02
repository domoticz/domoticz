import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { WindLogComponent } from './wind-log.component';
import { WindDayGraphComponent } from '../wind-day-graph/wind-day-graph.component';
import { WindDirectionGraphComponent } from '../wind-direction-graph/wind-direction-graph.component';
import { WindMonthGraphComponent } from '../wind-month-graph/wind-month-graph.component';
import { WindYearGraphComponent } from '../wind-year-graph/wind-year-graph.component';

describe('WindLogComponent', () => {
  let component: WindLogComponent;
  let fixture: ComponentFixture<WindLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        WindLogComponent,
        WindDayGraphComponent,
        WindDirectionGraphComponent,
        WindMonthGraphComponent,
        WindYearGraphComponent
      ],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(WindLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
