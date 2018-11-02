import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {HardwareComponent} from './hardware/hardware.component';
import {HardwareSetupComponent} from './hardware-setup/hardware-setup.component';
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
import {OfflineGuard} from '../../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../../_shared/_guards/authentication.guard';

const routes: Routes = [
  {
    path: '',
    component: HardwareComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: ':idx',
    component: HardwareSetupComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: ':idx/ZWaveUserCode/:nodeidx',
    component: HardwareZWaveUserCodeComponentComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditRFXCOMMode/:idx',
    component: HardwareEditRfxcomModeComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditRFXCOMMode868/:idx',
    component: HardwareEditRfxcomModeEightsixeightComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditS0MeterType/:idx',
    component: HardwareEditSzeroMeterTypeComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditLimitlessType/:idx',
    component: HardwareEditLimitlessTypeComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditSBFSpot/:idx',
    component: HardwareEditSbfSpotComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditOpenTherm/:idx',
    component: HardwareEditOpenTermComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditLMS/:idx',
    component: HardwareEditLmsComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditRego6XXType/:idx',
    component: EditRego6xxTypeComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'EditCCUSB/:idx',
    component: HardwareEditCcusbComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'TellstickSettings/:idx',
    component: HardwareTellstickSettingsComponent,
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class HardwareRoutingModule {
}
