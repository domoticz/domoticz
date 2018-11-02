import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CurrentDayGraphComponent } from './current-day-graph.component';
import { CommonTestModule } from '../../../_testing/common.test.module';

describe('CurrentDayGraphComponent', () => {
  let component: CurrentDayGraphComponent;
  let fixture: ComponentFixture<CurrentDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CurrentDayGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CurrentDayGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
