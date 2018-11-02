import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceLevelRenameModalComponent } from './device-level-rename-modal.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('DeviceLevelRenameModalComponent', () => {
  let component: DeviceLevelRenameModalComponent;
  let fixture: ComponentFixture<DeviceLevelRenameModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DeviceLevelRenameModalComponent ],
      imports: [CommonTestModule, FormsModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceLevelRenameModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
