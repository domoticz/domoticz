import {ModuleWithProviders, NgModule} from '@angular/core';
import {FormsModule} from '@angular/forms';
import {CommonModule} from '@angular/common';
import {AddEditFloorplanDialogComponent} from './_dialogs/add-edit-floorplan-dialog/add-edit-floorplan-dialog.component';
import {AddEditPlanDialogComponent} from './_dialogs/add-edit-plan-dialog/add-edit-plan-dialog.component';
import {AddEditCameraDialogComponent} from './_dialogs/add-edit-camera-dialog/add-edit-camera-dialog.component';
import {FindlatlongDialogComponent} from './_dialogs/findlatlong-dialog/findlatlong-dialog.component';
import {CreateDummySensorDialogComponent} from './_dialogs/create-dummy-sensor-dialog/create-dummy-sensor-dialog.component';
import {AddYeeLightDialogComponent} from './_dialogs/add-yee-light-dialog/add-yee-light-dialog.component';
import {CreateRfLinkDeviceDialogComponent} from './_dialogs/create-rf-link-device-dialog/create-rf-link-device-dialog.component';
import {CreateEvohomeSensorsDialogComponent} from './_dialogs/create-evohome-sensors-dialog/create-evohome-sensors-dialog.component';
import {AddAriluxDialogComponent} from './_dialogs/add-arilux-dialog/add-arilux-dialog.component';
import {EditCustomSensorDeviceDialogComponent} from './_dialogs/edit-custom-sensor-device-dialog/edit-custom-sensor-device-dialog.component';
import {EditDistanceDeviceDialogComponent} from './_dialogs/edit-distance-device-dialog/edit-distance-device-dialog.component';
import {EditEnergyDeviceDialogComponent} from './_dialogs/edit-energy-device-dialog/edit-energy-device-dialog.component';
import {EditMeterDeviceDialogComponent} from './_dialogs/edit-meter-device-dialog/edit-meter-device-dialog.component';
import {EditSetpointDeviceDialogComponent} from './_dialogs/edit-setpoint-device-dialog/edit-setpoint-device-dialog.component';
import {EditThermostatClockDeviceDialogComponent} from './_dialogs/edit-thermostat-clock-device-dialog/edit-thermostat-clock-device-dialog.component';
import {EditThermostatModeDialogComponent} from './_dialogs/edit-thermostat-mode-dialog/edit-thermostat-mode-dialog.component';
import {EditUtilityDeviceDialogComponent} from './_dialogs/edit-utility-device-dialog/edit-utility-device-dialog.component';
import {EditRainDeviceDialogComponent} from './_dialogs/edit-rain-device-dialog/edit-rain-device-dialog.component';
import {EditVisibilityDeviceDialogComponent} from './_dialogs/edit-visibility-device-dialog/edit-visibility-device-dialog.component';
import {EditBaroDeviceDialogComponent} from './_dialogs/edit-baro-device-dialog/edit-baro-device-dialog.component';
import {EditWeatherDeviceDialogComponent} from './_dialogs/edit-weather-device-dialog/edit-weather-device-dialog.component';
import {EditTempDeviceDialogComponent} from './_dialogs/edit-temp-device-dialog/edit-temp-device-dialog.component';
import {EditSetPointDialogComponent} from './_dialogs/edit-set-point-dialog/edit-set-point-dialog.component';
import {EditStateDialogComponent} from './_dialogs/edit-state-dialog/edit-state-dialog.component';
import {DialogService} from './_services/dialog.service';
import {ReplaceDeviceService} from './_services/replace-device.service';
import {ReplaceDeviceDialogComponent} from './_dialogs/replace-device-dialog/replace-device-dialog.component';
import {ApiService} from './_services/api.service';
import {ConfigService} from './_services/config.service';
import {LivesocketService} from './_services/livesocket.service';
import {NotifyBrowserService} from './_services/notify-browser.service';
import {ProtectionService} from './_services/protection.service';
import {TimesunService} from './_services/timesun.service';
import {NotificationService} from './_services/notification.service';
import {NotificationComponent} from './_components/notification/notification.component';
import {ApiHelperService} from './_services/api-helper.service';
import {PermissionService} from './_services/permission.service';
import {HasLoginDirective} from './_directives/has-login.directive';
import {HasLoginNoAdminDirective} from './_directives/has-login-no-admin.directive';
import {HasPermissionDirective} from './_directives/has-permission.directive';
import {HasUserDirective} from './_directives/has-user.directive';
import {BackButtonDirective} from './_directives/backbutton';
import {DateTimePickerDirective} from './_directives/datetimepicker';
import {DatePickerDirective} from './_directives/datepicker';
import {DragDropDirective} from './_directives/dragdrop';
import {TranslatePipe} from './_pipes/translate.pipe';
import {RoomService} from './_services/room.service';
import {AlertareaComponent} from './_components/alertarea/alertarea.component';
import {BackToTopComponent} from './_components/back-to-top/back-to-top.component';
import {PageLoadingIndicatorComponent} from './_components/page-loading-indicator/page-loading-indicator.component';
import {RgbwPickerComponent} from './_components/rgbw-picker/rgbw-picker.component';
import {RoomPlansSelectorComponent} from './_components/room-plans-selector/room-plans-selector.component';
import {TimesunComponent} from './_components/timesun/timesun.component';
import {RgbwPickerService} from './_components/rgbw-picker/rgbw-picker.service';
import {DimmerSliderComponent} from './_components/dimmer-slider/dimmer-slider.component';
import {IthoPopupComponent} from './_components/itho-popup/itho-popup.component';
import {LucciPopupComponent} from './_components/lucci-popup/lucci-popup.component';
import {RgbwPopupComponent} from './_components/rgbw-popup/rgbw-popup.component';
import {SelectorlevelsComponent} from './_components/selectorlevels/selectorlevels.component';
import {Therm3PopupComponent} from './_components/therm3-popup/therm3-popup.component';
import {ArmSystemDialogComponent} from './_dialogs/arm-system-dialog/arm-system-dialog.component';
import {AddLightDeviceDialogComponent} from './_dialogs/add-light-device-dialog/add-light-device-dialog.component';
import {AddManualLightDeviceDialogComponent} from './_dialogs/add-manual-light-device-dialog/add-manual-light-device-dialog.component';
import {LightSwitchesService} from './_services/light-switches.service';
import {DeviceService} from './_services/device.service';
import {DomoticzDevicesService} from './_services/domoticz-devices.service';
import {MediaRemoteDialogComponent} from './_dialogs/media-remote-dialog/media-remote-dialog.component';
import {CameraLiveDialogComponent} from './_dialogs/camera-live-dialog/camera-live-dialog.component';
import {LmsPlayerRemoteDialogComponent} from './_dialogs/lms-player-remote-dialog/lms-player-remote-dialog.component';
import {HeosPlayerRemoteDialogComponent} from './_dialogs/heos-player-remote-dialog/heos-player-remote-dialog.component';
import {TimersTableComponent} from './_components/timers-table/timers-table.component';
import {TimerFormComponent} from './_components/timer-form/timer-form.component';
import {SetpointPopupComponent} from './_components/setpoint-popup/setpoint-popup.component';
import {SetpointService} from './_services/setpoint.service';
import {AddSceneDialogComponent} from './_dialogs/add-scene-dialog/add-scene-dialog.component';
import {SceneService} from './_services/scene.service';
import {FloorplansService} from './_services/floorplans.service';
import {DeviceTextLogTableComponent} from './_components/device-text-log-table/device-text-log-table.component';
import {CamService} from './_services/cam.service';
import {DeviceLightService} from './_services/device-light.service';
import {DeviceTimerConfigUtilsService} from './_services/device-timer-config-utils.service';
import {DeviceTimerOptionsService} from "./_services/device-timer-options.service";
import {GraphService} from "./_services/graph.service";
import {HardwareService} from "./_services/hardware.service";
import {InitService} from "./_services/init.service";
import {LanguageService} from "./_services/language.service";
import {LoginService} from "./_services/login.service";
import {SetupService} from "./_services/setup.service";
import {UpdateService} from "./_services/update.service";
import {UserVariablesService} from "./_services/user-variables.service";
import {UsersService} from "./_services/users.service";

@NgModule({
  declarations: [
    AddEditFloorplanDialogComponent,
    AddEditPlanDialogComponent,
    AddEditCameraDialogComponent,
    FindlatlongDialogComponent,
    CreateDummySensorDialogComponent,
    AddYeeLightDialogComponent,
    CreateRfLinkDeviceDialogComponent,
    CreateEvohomeSensorsDialogComponent,
    AddAriluxDialogComponent,
    EditCustomSensorDeviceDialogComponent,
    EditDistanceDeviceDialogComponent,
    EditEnergyDeviceDialogComponent,
    EditMeterDeviceDialogComponent,
    EditSetpointDeviceDialogComponent,
    EditThermostatClockDeviceDialogComponent,
    EditThermostatModeDialogComponent,
    EditUtilityDeviceDialogComponent,
    EditRainDeviceDialogComponent,
    EditVisibilityDeviceDialogComponent,
    EditBaroDeviceDialogComponent,
    EditWeatherDeviceDialogComponent,
    EditTempDeviceDialogComponent,
    EditSetPointDialogComponent,
    EditStateDialogComponent,
    ReplaceDeviceDialogComponent,
    NotificationComponent,
    HasLoginDirective,
    HasLoginNoAdminDirective,
    HasPermissionDirective,
    HasUserDirective,
    BackButtonDirective,
    DateTimePickerDirective,
    DatePickerDirective,
    DragDropDirective,
    TranslatePipe,
    AlertareaComponent,
    BackToTopComponent,
    PageLoadingIndicatorComponent,
    RgbwPickerComponent,
    RoomPlansSelectorComponent,
    TimesunComponent,
    DimmerSliderComponent,
    IthoPopupComponent,
    LucciPopupComponent,
    RgbwPopupComponent,
    SelectorlevelsComponent,
    Therm3PopupComponent,
    ArmSystemDialogComponent,
    AddLightDeviceDialogComponent,
    AddManualLightDeviceDialogComponent,
    MediaRemoteDialogComponent,
    CameraLiveDialogComponent,
    LmsPlayerRemoteDialogComponent,
    HeosPlayerRemoteDialogComponent,
    TimersTableComponent,
    TimerFormComponent,
    SetpointPopupComponent,
    AddSceneDialogComponent,
    DeviceTextLogTableComponent
  ],
  imports: [
    CommonModule,
    FormsModule,
  ],
  providers: [],
  exports: [
    AddEditFloorplanDialogComponent,
    AddEditPlanDialogComponent,
    AddEditCameraDialogComponent,
    FindlatlongDialogComponent,
    CreateDummySensorDialogComponent,
    AddYeeLightDialogComponent,
    CreateRfLinkDeviceDialogComponent,
    CreateEvohomeSensorsDialogComponent,
    AddAriluxDialogComponent,
    EditCustomSensorDeviceDialogComponent,
    EditDistanceDeviceDialogComponent,
    EditEnergyDeviceDialogComponent,
    EditMeterDeviceDialogComponent,
    EditSetpointDeviceDialogComponent,
    EditThermostatClockDeviceDialogComponent,
    EditThermostatModeDialogComponent,
    EditUtilityDeviceDialogComponent,
    EditRainDeviceDialogComponent,
    EditVisibilityDeviceDialogComponent,
    EditBaroDeviceDialogComponent,
    EditWeatherDeviceDialogComponent,
    EditTempDeviceDialogComponent,
    EditSetPointDialogComponent,
    EditStateDialogComponent,
    ReplaceDeviceDialogComponent,
    NotificationComponent,
    HasLoginDirective,
    HasLoginNoAdminDirective,
    HasPermissionDirective,
    HasUserDirective,
    BackButtonDirective,
    DateTimePickerDirective,
    DatePickerDirective,
    DragDropDirective,
    TranslatePipe,
    AlertareaComponent,
    BackToTopComponent,
    PageLoadingIndicatorComponent,
    RgbwPickerComponent,
    RoomPlansSelectorComponent,
    TimesunComponent,
    DimmerSliderComponent,
    IthoPopupComponent,
    LucciPopupComponent,
    RgbwPopupComponent,
    SelectorlevelsComponent,
    Therm3PopupComponent,
    ArmSystemDialogComponent,
    AddLightDeviceDialogComponent,
    AddManualLightDeviceDialogComponent,
    MediaRemoteDialogComponent,
    CameraLiveDialogComponent,
    LmsPlayerRemoteDialogComponent,
    HeosPlayerRemoteDialogComponent,
    TimersTableComponent,
    TimerFormComponent,
    SetpointPopupComponent,
    AddSceneDialogComponent,
    DeviceTextLogTableComponent
  ]
})
export class SharedModule {

  static forRoot(): ModuleWithProviders {
    return {
      ngModule: SharedModule,
      providers: [
        ApiService,
        ConfigService,
        LivesocketService,
        NotifyBrowserService,
        ProtectionService,
        TimesunService,
        DialogService,
        ReplaceDeviceService,
        NotificationService,
        ApiHelperService,
        PermissionService,
        RoomService,
        RgbwPickerService,
        LightSwitchesService,
        DeviceService,
        DomoticzDevicesService,
        SetpointService,
        SceneService,
        FloorplansService,
        CamService,
        DeviceLightService,
        DeviceTimerConfigUtilsService,
        DeviceTimerOptionsService,
        GraphService,
        HardwareService,
        InitService,
        LanguageService,
        LoginService,
        SetupService,
        UpdateService,
        UserVariablesService,
        UsersService,
      ]
    };
  }

}
