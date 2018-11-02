import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {Therm3PopupComponent} from './therm3-popup.component';
import {CommonTestModule} from '../../../_testing/common.test.module';

describe('Therm3PopupComponent', () => {
  let component: Therm3PopupComponent;
  let fixture: ComponentFixture<Therm3PopupComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(Therm3PopupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
