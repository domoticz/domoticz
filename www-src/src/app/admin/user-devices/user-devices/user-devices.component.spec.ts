import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { UserDevicesComponent } from './user-devices.component';
import { CommonTestModule } from '../../../_testing/common.test.module';

describe('UserDevicesComponent', () => {
  let component: UserDevicesComponent;
  let fixture: ComponentFixture<UserDevicesComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [UserDevicesComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UserDevicesComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
