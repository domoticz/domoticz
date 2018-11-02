import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DevicesComponent} from './devices.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {DeviceFilterByUsageComponent} from '../device-filter-by-usage/device-filter-by-usage.component';
import {DevicesFiltersComponent} from '../devices-filters/devices-filters.component';
import {DevicesTableComponent} from '../devices-table/devices-table.component';
import {FormsModule} from '@angular/forms';
import {DeviceRenameModalComponent} from '../device-rename-modal/device-rename-modal.component';
import {DeviceAddModalComponent} from '../device-add-modal/device-add-modal.component';

describe('DevicesComponent', () => {
  let component: DevicesComponent;
  let fixture: ComponentFixture<DevicesComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        DevicesComponent,
        DeviceFilterByUsageComponent,
        DevicesFiltersComponent,
        DevicesTableComponent,
        DeviceAddModalComponent,
        DeviceRenameModalComponent
      ],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DevicesComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
