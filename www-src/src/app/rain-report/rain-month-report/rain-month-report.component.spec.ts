import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RainMonthReportComponent } from './rain-month-report.component';
import { ActivatedRoute, convertToParamMap } from '@angular/router';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('RainMonthReportComponent', () => {
  let component: RainMonthReportComponent;
  let fixture: ComponentFixture<RainMonthReportComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RainMonthReportComponent],
      imports: [CommonTestModule],
      providers: [
        {
          provide: ActivatedRoute,
          useValue: {
            snapshot: {
              paramMap: convertToParamMap({ idx: '1' })
            }
          }
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RainMonthReportComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
