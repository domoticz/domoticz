import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { SmartDayGraphComponent } from './smart-day-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('SmartDayGraphComponent', () => {
  let component: SmartDayGraphComponent;
  let fixture: ComponentFixture<SmartDayGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [SmartDayGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SmartDayGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';
    component.switchtype = 1;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
