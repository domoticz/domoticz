import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditRfxcomModeComponent } from './hardware-edit-rfxcom-mode.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareEditRfxcomModeComponent', () => {
  let component: HardwareEditRfxcomModeComponent;
  let fixture: ComponentFixture<HardwareEditRfxcomModeComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ HardwareEditRfxcomModeComponent ],
      imports: [CommonTestModule, FormsModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareEditRfxcomModeComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
