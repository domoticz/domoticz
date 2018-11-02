import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceLevelNamesTableComponent } from './device-level-names-table.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { DeviceLevelRenameModalComponent } from '../device-level-rename-modal/device-level-rename-modal.component';
import { FormsModule } from '@angular/forms';

describe('DeviceLevelNamesTableComponent', () => {
  let component: DeviceLevelNamesTableComponent;
  let fixture: ComponentFixture<DeviceLevelNamesTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceLevelNamesTableComponent, DeviceLevelRenameModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceLevelNamesTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
