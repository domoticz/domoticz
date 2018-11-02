import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {PageLoadingIndicatorComponent} from './page-loading-indicator.component';
import {CommonTestModule} from '../../../_testing/common.test.module';

describe('PageLoadingIndicatorComponent', () => {
  let component: PageLoadingIndicatorComponent;
  let fixture: ComponentFixture<PageLoadingIndicatorComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(PageLoadingIndicatorComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
