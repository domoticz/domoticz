import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceLevelActionsTableComponent } from './device-level-actions-table.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { DeviceEditSelectorActionModalComponent } from '../device-edit-selector-action-modal/device-edit-selector-action-modal.component';
import { FormsModule } from '@angular/forms';

describe('DeviceLevelActionsTableComponent', () => {
  let component: DeviceLevelActionsTableComponent;
  let fixture: ComponentFixture<DeviceLevelActionsTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceLevelActionsTableComponent, DeviceEditSelectorActionModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceLevelActionsTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
