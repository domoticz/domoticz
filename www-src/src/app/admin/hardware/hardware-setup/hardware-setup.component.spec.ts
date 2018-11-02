import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareSetupComponent } from './hardware-setup.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { ZwaveHardwareComponent } from '../zwave-hardware/zwave-hardware.component';
import { WolHardwareComponent } from '../wol-hardware/wol-hardware.component';
import { PingerComponent } from '../pinger/pinger.component';
import { MySensorsComponent } from '../my-sensors/my-sensors.component';
import { KodiComponent } from '../kodi/kodi.component';
import { PanasonicTvHardwareComponent } from '../panasonic-tv-hardware/panasonic-tv-hardware.component';
import { BleBoxHardwareComponent } from '../ble-box-hardware/ble-box-hardware.component';
import { FormsModule } from '@angular/forms';
import { ZWaveExcludeModalComponent } from '../z-wave-exclude-modal/z-wave-exclude-modal.component';
import { ZWaveIncludeModalComponent } from '../z-wave-include-modal/z-wave-include-modal.component';

describe('HardwareSetupComponent', () => {
  let component: HardwareSetupComponent;
  let fixture: ComponentFixture<HardwareSetupComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        HardwareSetupComponent,
        ZwaveHardwareComponent,
        ZWaveExcludeModalComponent,
        ZWaveIncludeModalComponent,
        WolHardwareComponent,
        MySensorsComponent,
        PingerComponent,
        KodiComponent,
        PanasonicTvHardwareComponent,
        BleBoxHardwareComponent
      ],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareSetupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
