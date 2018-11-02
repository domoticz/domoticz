import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RainReportComponent } from './rain-report.component';
import { CommonTestModule } from '../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';
import { ActivatedRoute, ParamMap, convertToParamMap } from '@angular/router';

describe('RainReportComponent', () => {
  let component: RainReportComponent;
  let fixture: ComponentFixture<RainReportComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RainReportComponent],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {
          provide: ActivatedRoute,
          useValue: {
            snapshot: {
              paramMap: convertToParamMap({ idx: '1'})
            }
          }
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RainReportComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
