import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { UvDayGraphComponent } from './uv-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('UvDayGraphComponent', () => {
  let component: UvDayGraphComponent;
  let fixture: ComponentFixture<UvDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [UvDayGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UvDayGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
