import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { FanDayGraphComponent } from './fan-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('FanDayGraphComponent', () => {
  let component: FanDayGraphComponent;
  let fixture: ComponentFixture<FanDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ FanDayGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FanDayGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
