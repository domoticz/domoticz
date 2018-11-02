import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { UvYearGraphComponent } from './uv-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('UvYearGraphComponent', () => {
  let component: UvYearGraphComponent;
  let fixture: ComponentFixture<UvYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [UvYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UvYearGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
