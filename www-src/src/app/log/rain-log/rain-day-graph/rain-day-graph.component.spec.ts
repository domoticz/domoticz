import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RainDayGraphComponent } from './rain-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('RainDayGraphComponent', () => {
  let component: RainDayGraphComponent;
  let fixture: ComponentFixture<RainDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RainDayGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RainDayGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
