import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DeviceTextLogComponent} from './device-text-log.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('DeviceTextLogComponent', () => {
  let component: DeviceTextLogComponent;
  let fixture: ComponentFixture<DeviceTextLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceTextLogComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceTextLogComponent);
    component = fixture.componentInstance;

    component.deviceIdx = '1';

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
