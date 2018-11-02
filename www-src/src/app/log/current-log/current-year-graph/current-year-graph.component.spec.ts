import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CurrentYearGraphComponent } from './current-year-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('CurrentYearGraphComponent', () => {
  let component: CurrentYearGraphComponent;
  let fixture: ComponentFixture<CurrentYearGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ CurrentYearGraphComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CurrentYearGraphComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
