import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {IthoPopupComponent} from './itho-popup.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';

describe('IthoPopupComponent', () => {
  let component: IthoPopupComponent;
  let fixture: ComponentFixture<IthoPopupComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(IthoPopupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
