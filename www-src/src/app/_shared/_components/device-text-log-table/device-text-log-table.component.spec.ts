import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DeviceTextLogTableComponent} from './device-text-log-table.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';

describe('DeviceTextLogTableComponent', () => {
  let component: DeviceTextLogTableComponent;
  let fixture: ComponentFixture<DeviceTextLogTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceTextLogTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
