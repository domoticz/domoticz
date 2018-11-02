import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {HardwareRoutingModule} from './hardware-routing.module';
import {HardwareComponent} from './hardware/hardware.component';
import {FormsModule} from '@angular/forms';
import {HardwareSetupComponent} from './hardware-setup/hardware-setup.component';
import {ZwaveHardwareComponent} from './zwave-hardware/zwave-hardware.component';
import {ZWaveIncludeModalComponent} from './z-wave-include-modal/z-wave-include-modal.component';
import {ZWaveExcludeModalComponent} from './z-wave-exclude-modal/z-wave-exclude-modal.component';
import {WolHardwareComponent} from './wol-hardware/wol-hardware.component';
import {MySensorsComponent} from './my-sensors/my-sensors.component';
import {PingerComponent} from './pinger/pinger.component';
import {KodiComponent} from './kodi/kodi.component';
import {PanasonicTvHardwareComponent} from './panasonic-tv-hardware/panasonic-tv-hardware.component';
import {BleBoxHardwareComponent} from './ble-box-hardware/ble-box-hardware.component';
import {HardwareZWaveUserCodeComponentComponent} from './hardware-z-wave-user-code-component/hardware-z-wave-user-code-component.component';
import {HardwareEditRfxcomModeComponent} from './hardware-edit-rfxcom-mode/hardware-edit-rfxcom-mode.component';
import {HardwareEditRfxcomModeEightsixeightComponent} from './hardware-edit-rfxcom-mode-eightsixeight/hardware-edit-rfxcom-mode-eightsixeight.component';
import {HardwareEditSzeroMeterTypeComponent} from './hardware-edit-szero-meter-type/hardware-edit-szero-meter-type.component';
import {HardwareEditLimitlessTypeComponent} from './hardware-edit-limitless-type/hardware-edit-limitless-type.component';
import {HardwareEditSbfSpotComponent} from './hardware-edit-sbf-spot/hardware-edit-sbf-spot.component';
import {HardwareEditOpenTermComponent} from './hardware-edit-open-term/hardware-edit-open-term.component';
import {HardwareEditLmsComponent} from './hardware-edit-lms/hardware-edit-lms.component';
import {EditRego6xxTypeComponent} from './edit-rego6xx-type/edit-rego6xx-type.component';
import {HardwareEditCcusbComponent} from './hardware-edit-ccusb/hardware-edit-ccusb.component';
import {HardwareTellstickSettingsComponent} from './hardware-tellstick-settings/hardware-tellstick-settings.component';
import {SharedModule} from '../../_shared/shared.module';
import {BleBoxService} from './ble-box.service';
import {KodiService} from './kodi.service';
import {MySensorsService} from './my-sensors.service';
import {PanasonicTvService} from './panasonic-tv.service';
import {PingerService} from './pinger.service';
import {WolService} from './wol.service';
import {ZwaveService} from './zwave.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    HardwareRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    HardwareComponent,
    HardwareSetupComponent,
    ZwaveHardwareComponent,
    ZWaveIncludeModalComponent,
    ZWaveExcludeModalComponent,
    WolHardwareComponent,
    MySensorsComponent,
    PingerComponent,
    KodiComponent,
    PanasonicTvHardwareComponent,
    BleBoxHardwareComponent,
    HardwareZWaveUserCodeComponentComponent,
    HardwareEditRfxcomModeComponent,
    HardwareEditRfxcomModeEightsixeightComponent,
    HardwareEditSzeroMeterTypeComponent,
    HardwareEditLimitlessTypeComponent,
    HardwareEditSbfSpotComponent,
    HardwareEditOpenTermComponent,
    HardwareEditLmsComponent,
    EditRego6xxTypeComponent,
    HardwareEditCcusbComponent,
    HardwareTellstickSettingsComponent
  ],
  exports: [],
  providers: [
    BleBoxService,
    KodiService,
    MySensorsService,
    PanasonicTvService,
    PingerService,
    WolService,
    ZwaveService
  ]
})
export class HardwareModule {
}
