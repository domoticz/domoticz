import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceEditSelectorActionModalComponent } from './device-edit-selector-action-modal.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('DeviceEditSelectorActionModalComponent', () => {
  let component: DeviceEditSelectorActionModalComponent;
  let fixture: ComponentFixture<DeviceEditSelectorActionModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DeviceEditSelectorActionModalComponent ],
      imports: [CommonTestModule, FormsModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceEditSelectorActionModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
