import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { WindYearGraphComponent } from './wind-year-graph.component';

describe('WindYearGraphComponent', () => {
  let component: WindYearGraphComponent;
  let fixture: ComponentFixture<WindYearGraphComponent>;

  const lscales = [
    { from: 0, to: 1 },
    { from: 1, to: 2 },
    { from: 2, to: 3 },
    { from: 3, to: 4 },
    { from: 4, to: 5 },
    { from: 5, to: 6 },
    { from: 6, to: 7 },
    { from: 7, to: 8 },
    { from: 8, to: 9 },
    { from: 9, to: 10 },
    { from: 10, to: 11 },
    { from: 11, to: 12 },
    { from: 12, to: 100 },
  ];

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [WindYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(WindYearGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';
    component.lscales = lscales;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
