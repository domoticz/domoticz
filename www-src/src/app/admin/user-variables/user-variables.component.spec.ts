import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {UserVariablesComponent} from './user-variables.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('UserVariablesComponent', () => {
  let component: UserVariablesComponent;
  let fixture: ComponentFixture<UserVariablesComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [UserVariablesComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UserVariablesComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
