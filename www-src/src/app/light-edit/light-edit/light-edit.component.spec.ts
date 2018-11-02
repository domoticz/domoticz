import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {LightEditComponent} from './light-edit.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {DeviceSubdevicesEditorComponent} from '../device-subdevices-editor/device-subdevices-editor.component';
import {DeviceColorSettingsComponent} from '../device-color-settings/device-color-settings.component';
import {DeviceLevelNamesTableComponent} from '../device-level-names-table/device-level-names-table.component';
import {DeviceLevelActionsTableComponent} from '../device-level-actions-table/device-level-actions-table.component';
import {DeviceLevelRenameModalComponent} from '../device-level-rename-modal/device-level-rename-modal.component';
import {DeviceEditSelectorActionModalComponent} from '../device-edit-selector-action-modal/device-edit-selector-action-modal.component';
import {FormsModule} from '@angular/forms';
import {DeviceIconSelectComponent} from '../device-icon-select/device-icon-select.component';

describe('LightEditComponent', () => {
  let component: LightEditComponent;
  let fixture: ComponentFixture<LightEditComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        LightEditComponent,
        DeviceSubdevicesEditorComponent,
        DeviceColorSettingsComponent,
        DeviceLevelNamesTableComponent,
        DeviceLevelRenameModalComponent,
        DeviceLevelActionsTableComponent,
        DeviceEditSelectorActionModalComponent,
        DeviceIconSelectComponent,
      ],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(LightEditComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
