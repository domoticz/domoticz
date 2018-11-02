import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { BarometerDayGraphComponent } from './barometer-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('BarometerDayGraphComponent', () => {
  let component: BarometerDayGraphComponent;
  let fixture: ComponentFixture<BarometerDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ BarometerDayGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(BarometerDayGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
