import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CurrentMonthGraphComponent } from './current-month-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CurrentMonthGraphComponent', () => {
  let component: CurrentMonthGraphComponent;
  let fixture: ComponentFixture<CurrentMonthGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CurrentMonthGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CurrentMonthGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
