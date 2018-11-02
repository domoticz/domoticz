import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareTellstickSettingsComponent } from './hardware-tellstick-settings.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareTellstickSettingsComponent', () => {
  let component: HardwareTellstickSettingsComponent;
  let fixture: ComponentFixture<HardwareTellstickSettingsComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareTellstickSettingsComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareTellstickSettingsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
