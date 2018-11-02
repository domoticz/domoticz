import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceIconSelectComponent } from './device-icon-select.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('DeviceIconSelectComponent', () => {
  let component: DeviceIconSelectComponent;
  let fixture: ComponentFixture<DeviceIconSelectComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DeviceIconSelectComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceIconSelectComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
