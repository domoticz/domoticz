import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CustomComponent} from './custom.component';
import {CommonTestModule} from '../../_testing/common.test.module';

describe('CustomComponent', () => {
  let component: CustomComponent;
  let fixture: ComponentFixture<CustomComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [CustomComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CustomComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
