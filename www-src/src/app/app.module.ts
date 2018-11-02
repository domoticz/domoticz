// tslint:disable:max-line-length
import {AppRoutingModule} from './app-routing.module';
import {BrowserModule} from '@angular/platform-browser';
import {BrowserAnimationsModule} from '@angular/platform-browser/animations';
import {FormsModule} from '@angular/forms';
import {APP_INITIALIZER, LOCALE_ID, NgModule} from '@angular/core';
import {HTTP_INTERCEPTORS, HttpClientModule} from '@angular/common/http';
import {I18NEXT_SERVICE, I18NextModule, ITranslationService} from 'angular-i18next';
import {TooltipModule} from 'ngx-tooltip';
import {AppComponent} from './app.component';
import {HeaderComponent} from './header/header.component';
import {UnauthorizedInterceptor} from './_shared/_interceptors/unauthorized.interceptor';
import {OfflineGuard} from './_shared/_guards/offline.guard';
import {AuthenticationGuard} from './_shared/_guards/authentication.guard';
import {EditTempDeviceDialogComponent} from './_shared/_dialogs/edit-temp-device-dialog/edit-temp-device-dialog.component';
import {EditSetPointDialogComponent} from './_shared/_dialogs/edit-set-point-dialog/edit-set-point-dialog.component';
import {EditStateDialogComponent} from './_shared/_dialogs/edit-state-dialog/edit-state-dialog.component';
import {InitService} from './_shared/_services/init.service';
import {EditRainDeviceDialogComponent} from './_shared/_dialogs/edit-rain-device-dialog/edit-rain-device-dialog.component';
import {EditVisibilityDeviceDialogComponent} from './_shared/_dialogs/edit-visibility-device-dialog/edit-visibility-device-dialog.component';
import {EditBaroDeviceDialogComponent} from './_shared/_dialogs/edit-baro-device-dialog/edit-baro-device-dialog.component';
import {EditWeatherDeviceDialogComponent} from './_shared/_dialogs/edit-weather-device-dialog/edit-weather-device-dialog.component';
import {EditUtilityDeviceDialogComponent} from './_shared/_dialogs/edit-utility-device-dialog/edit-utility-device-dialog.component';
import {EditMeterDeviceDialogComponent} from './_shared/_dialogs/edit-meter-device-dialog/edit-meter-device-dialog.component';
import {EditEnergyDeviceDialogComponent} from './_shared/_dialogs/edit-energy-device-dialog/edit-energy-device-dialog.component';
import {EditCustomSensorDeviceDialogComponent} from './_shared/_dialogs/edit-custom-sensor-device-dialog/edit-custom-sensor-device-dialog.component';
import {EditDistanceDeviceDialogComponent} from './_shared/_dialogs/edit-distance-device-dialog/edit-distance-device-dialog.component';
import {EditThermostatClockDeviceDialogComponent} from './_shared/_dialogs/edit-thermostat-clock-device-dialog/edit-thermostat-clock-device-dialog.component';
import {EditThermostatModeDialogComponent} from './_shared/_dialogs/edit-thermostat-mode-dialog/edit-thermostat-mode-dialog.component';
import {EditSetpointDeviceDialogComponent} from './_shared/_dialogs/edit-setpoint-device-dialog/edit-setpoint-device-dialog.component';
import {LoginComponent} from './auth/login/login.component';
import {LogoutComponent} from './auth/logout/logout.component';
import {OfflineComponent} from './offline/offline.component';
import {RoundProgressModule} from 'angular-svg-round-progressbar';
import {CreateDummySensorDialogComponent} from './_shared/_dialogs/create-dummy-sensor-dialog/create-dummy-sensor-dialog.component';
import {AddYeeLightDialogComponent} from './_shared/_dialogs/add-yee-light-dialog/add-yee-light-dialog.component';
import {CreateRfLinkDeviceDialogComponent} from './_shared/_dialogs/create-rf-link-device-dialog/create-rf-link-device-dialog.component';
import {CreateEvohomeSensorsDialogComponent} from './_shared/_dialogs/create-evohome-sensors-dialog/create-evohome-sensors-dialog.component';
import {AddAriluxDialogComponent} from './_shared/_dialogs/add-arilux-dialog/add-arilux-dialog.component';
import {AddEditCameraDialogComponent} from './_shared/_dialogs/add-edit-camera-dialog/add-edit-camera-dialog.component';
import {AddEditPlanDialogComponent} from './_shared/_dialogs/add-edit-plan-dialog/add-edit-plan-dialog.component';
import {AddEditFloorplanDialogComponent} from './_shared/_dialogs/add-edit-floorplan-dialog/add-edit-floorplan-dialog.component';
import {ReplaceDeviceDialogComponent} from './_shared/_dialogs/replace-device-dialog/replace-device-dialog.component';
import {MediaRemoteDialogComponent} from './_shared/_dialogs/media-remote-dialog/media-remote-dialog.component';
import {CameraLiveDialogComponent} from './_shared/_dialogs/camera-live-dialog/camera-live-dialog.component';
import {LmsPlayerRemoteDialogComponent} from './_shared/_dialogs/lms-player-remote-dialog/lms-player-remote-dialog.component';
import {HeosPlayerRemoteDialogComponent} from './_shared/_dialogs/heos-player-remote-dialog/heos-player-remote-dialog.component';
import {ArmSystemDialogComponent} from './_shared/_dialogs/arm-system-dialog/arm-system-dialog.component';
import {AddManualLightDeviceDialogComponent} from './_shared/_dialogs/add-manual-light-device-dialog/add-manual-light-device-dialog.component';
import {AddLightDeviceDialogComponent} from './_shared/_dialogs/add-light-device-dialog/add-light-device-dialog.component';
import {AddSceneDialogComponent} from './_shared/_dialogs/add-scene-dialog/add-scene-dialog.component';
import {FindlatlongDialogComponent} from './_shared/_dialogs/findlatlong-dialog/findlatlong-dialog.component';
import {SharedModule} from './_shared/shared.module';

export function initApp(initService: InitService): () => Promise<any> {
  return () => initService.init();
}

export function localeIdFactory(i18next: ITranslationService) {
  return i18next.language;
}

@NgModule({
  declarations: [
    AppComponent,
    HeaderComponent,
    LoginComponent,
    LogoutComponent,
    OfflineComponent,
  ],
  entryComponents: [
    /* Dialogs must be added here */
    AddAriluxDialogComponent,
    AddEditPlanDialogComponent,
    AddEditFloorplanDialogComponent,
    AddLightDeviceDialogComponent,
    AddManualLightDeviceDialogComponent,
    AddSceneDialogComponent,
    AddYeeLightDialogComponent,
    ArmSystemDialogComponent,
    CameraLiveDialogComponent,
    CreateEvohomeSensorsDialogComponent,
    CreateDummySensorDialogComponent,
    CreateRfLinkDeviceDialogComponent,
    EditBaroDeviceDialogComponent,
    EditCustomSensorDeviceDialogComponent,
    EditDistanceDeviceDialogComponent,
    EditEnergyDeviceDialogComponent,
    EditMeterDeviceDialogComponent,
    EditRainDeviceDialogComponent,
    EditSetpointDeviceDialogComponent,
    EditSetPointDialogComponent,
    EditStateDialogComponent,
    EditTempDeviceDialogComponent,
    EditThermostatClockDeviceDialogComponent,
    EditThermostatModeDialogComponent,
    EditUtilityDeviceDialogComponent,
    EditVisibilityDeviceDialogComponent,
    EditWeatherDeviceDialogComponent,
    HeosPlayerRemoteDialogComponent,
    LmsPlayerRemoteDialogComponent,
    MediaRemoteDialogComponent,
    ReplaceDeviceDialogComponent,
    FindlatlongDialogComponent,
    AddEditCameraDialogComponent
  ],
  imports: [
    /* Domoticz */
    AppRoutingModule,
    SharedModule.forRoot(),
    /* Angular */
    BrowserModule,
    BrowserAnimationsModule,
    HttpClientModule,
    FormsModule,
    /* 3rd party */
    I18NextModule.forRoot(),
    TooltipModule,
    RoundProgressModule
  ],
  providers: [
    /* Services choose themselves where to be injected */
    /* Guards */
    OfflineGuard,
    AuthenticationGuard,
    /* Interceptors */
    {
      provide: HTTP_INTERCEPTORS,
      useClass: UnauthorizedInterceptor,
      multi: true
    },
    /* App init */
    {
      provide: APP_INITIALIZER,
      useFactory: initApp,
      deps: [InitService],
      multi: true
    },
    /* i18n */
    {
      provide: LOCALE_ID,
      deps: [I18NEXT_SERVICE],
      useFactory: localeIdFactory
    }
  ],
  bootstrap: [AppComponent]
})
export class AppModule {
}
