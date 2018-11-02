import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RainYearGraphComponent } from './rain-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('RainYearGraphComponent', () => {
  let component: RainYearGraphComponent;
  let fixture: ComponentFixture<RainYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RainYearGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RainYearGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
