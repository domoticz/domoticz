import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceRenameModalComponent } from './device-rename-modal.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('DeviceRenameModalComponent', () => {
  let component: DeviceRenameModalComponent;
  let fixture: ComponentFixture<DeviceRenameModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceRenameModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceRenameModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
