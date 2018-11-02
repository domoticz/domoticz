import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceAddModalComponent } from './device-add-modal.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('DeviceAddModalComponent', () => {
  let component: DeviceAddModalComponent;
  let fixture: ComponentFixture<DeviceAddModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceAddModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceAddModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
