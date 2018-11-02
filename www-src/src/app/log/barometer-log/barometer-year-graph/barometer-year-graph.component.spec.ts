import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { BarometerYearGraphComponent } from './barometer-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('BarometerYearGraphComponent', () => {
  let component: BarometerYearGraphComponent;
  let fixture: ComponentFixture<BarometerYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ BarometerYearGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(BarometerYearGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
