import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DevicesTableComponent } from './devices-table.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { DeviceAddModalComponent } from '../device-add-modal/device-add-modal.component';
import { DeviceRenameModalComponent } from '../device-rename-modal/device-rename-modal.component';
import { FormsModule } from '@angular/forms';

describe('DevicesTableComponent', () => {
  let component: DevicesTableComponent;
  let fixture: ComponentFixture<DevicesTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DevicesTableComponent, DeviceAddModalComponent, DeviceRenameModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DevicesTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
