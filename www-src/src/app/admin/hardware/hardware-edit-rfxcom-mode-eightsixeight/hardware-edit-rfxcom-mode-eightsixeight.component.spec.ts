import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditRfxcomModeEightsixeightComponent } from './hardware-edit-rfxcom-mode-eightsixeight.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareEditRfxcomModeEightsixeightComponent', () => {
  let component: HardwareEditRfxcomModeEightsixeightComponent;
  let fixture: ComponentFixture<HardwareEditRfxcomModeEightsixeightComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareEditRfxcomModeEightsixeightComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareEditRfxcomModeEightsixeightComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
