import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {TimesunComponent} from './timesun.component';
import {CommonTestModule} from '../../../_testing/common.test.module';

describe('TimesunComponent', () => {
  let component: TimesunComponent;
  let fixture: ComponentFixture<TimesunComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TimesunComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
