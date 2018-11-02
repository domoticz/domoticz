import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { SmartYearGraphComponent } from './smart-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('SmartYearGraphComponent', () => {
  let component: SmartYearGraphComponent;
  let fixture: ComponentFixture<SmartYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [SmartYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SmartYearGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';
    component.switchtype = 1;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
