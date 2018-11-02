import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { FanYearGraphComponent } from './fan-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('FanYearGraphComponent', () => {
  let component: FanYearGraphComponent;
  let fixture: ComponentFixture<FanYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ FanYearGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FanYearGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
