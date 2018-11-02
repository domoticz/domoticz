import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { UvMonthGraphComponent } from './uv-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('UvMonthGraphComponent', () => {
  let component: UvMonthGraphComponent;
  let fixture: ComponentFixture<UvMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [UvMonthGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UvMonthGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
