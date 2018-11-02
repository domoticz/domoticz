// tslint:disable:max-line-length
import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {AuthenticationGuard} from './_shared/_guards/authentication.guard';
import {OfflineGuard} from './_shared/_guards/offline.guard';
import {LoginComponent} from './auth/login/login.component';
import {LogoutComponent} from './auth/logout/logout.component';
import {OfflineComponent} from './offline/offline.component';

const routes: Routes = [
  {path: '', redirectTo: '/Dashboard', pathMatch: 'full'},
  {
    path: 'dashboard',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./dashboard/dashboard.module').then(m => m.DashboardModule)
  },
  {
    path: 'Dashboard',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./dashboard/dashboard.module').then(m => m.DashboardModule)
  },
  {
    path: 'History',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./history/history.module').then(m => m.HistoryModule)
  },
  {
    path: 'Floorplans',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./floorplans/floorplans.module').then(m => m.FloorplansModule)
  },
  {
    path: 'LightSwitches',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./light-switches/light-switches.module').then(m => m.LightSwitchesModule)
  },
  {
    path: 'Lights',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./light-switches/light-switches.module').then(m => m.LightSwitchesModule)
  },
  {
    path: 'Devices/:idx/LightEdit',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./light-edit/light-edit.module').then(m => m.LightEditModule)
  },
  {
    path: 'Scenes',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./scenes/scenes.module').then(m => m.ScenesModule)
  },
  {
    path: 'Temperature',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./temperature/temperature.module').then(m => m.TemperatureModule)
  },
  {
    path: 'Devices/:idx/Log',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/device-log/device-log.module').then(m => m.DeviceLogModule)
  },
  {
    path: 'Devices/:idx/Notifications',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./device-notifications/device-notifications.module').then(m => m.DeviceNotificationsModule)
  },
  {
    path: 'Devices/:idx/Report',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./device-report/device-report.module').then(m => m.DeviceReportModule)
  },
  {
    path: 'Devices/:idx/Timers',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./device-timers/device-timers.module').then(m => m.DeviceTimersModule)
  },
  {
    path: 'Forecast',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./forecast/forecast.module').then(m => m.ForecastModule)
  },
  {
    path: 'Weather',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./weather/weather.module').then(m => m.WeatherModule)
  },
  {
    path: 'BaroLog/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/barometer-log/barometer-log.module').then(m => m.BarometerLogModule)
  },
  {
    path: 'RainLog/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/rain-log/rain-log.module').then(m => m.RainLogModule)
  },
  {
    path: 'RainReport/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./rain-report/rain-report.module').then(m => m.RainReportModule)
  },
  {
    path: 'UVLog/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/uv-log/uv-log.module').then(m => m.UvLogModule)
  },
  {
    path: 'WindLog/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/wind-log/wind-log.module').then(m => m.WindLogModule)
  },
  {
    path: 'Utility',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./utility/utility.module').then(m => m.UtilityModule),
  },
  {
    path: 'AirQualityLog/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/air-quality-log/air-quality-log.module').then(m => m.AirQualityLogModule),
  },
  {
    path: 'FanLog/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/fan-log/fan-log.module').then(m => m.FanLogModule),
  },
  {
    path: 'CurrentLog/:idx',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./log/current-log/current-log.module').then(m => m.CurrentLogModule),
  },
  {
    path: 'About',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/about/about.module').then(m => m.AboutModule),
  },
  {
    path: 'Frontpage',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./frontpage/frontpage.module').then(m => m.FrontpageModule),
  },
  {
    path: 'Custom/:custompage',
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./custom/custom.module').then(m => m.CustomModule),
  },
  {
    path: 'Devices',
    // data: { permission: 'Admin' },
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/devices/devices.module').then(m => m.DevicesModule),
  },
  // Admin pages
  {
    path: 'Hardware',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/hardware/hardware.module').then(m => m.HardwareModule),
  },
  {
    path: 'RFXComFirmware/:idx',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/rfxcom-firmware/rfxcom-firmware.module').then(m => m.RfxcomFirmwareModule),
  },
  {
    path: 'Setup',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/setup/setup.module').then(m => m.SetupModule),
  },
  {
    path: 'Update',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/update/update.module').then(m => m.UpdateModule),
  },
  {
    path: 'Cam',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/cam/cam.module').then(m => m.CamModule),
  },
  {
    path: 'CustomIcons',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/customicons/customicons.module').then(m => m.CustomiconsModule),
  },
  {
    path: 'Users',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/users/users.module').then(m => m.UsersModule),
  },
  {
    path: 'UserDevices/:idx/:name',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/user-devices/user-devices.module').then(m => m.UserDevicesModule),
  },
  {
    path: 'Events',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/events/events.module').then(m => m.EventsModule),
  },
  {
    path: 'Mobile',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/mobile/mobile.module').then(m => m.MobileModule),
  },
  {
    path: 'Roomplan',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/roomplan/roomplan.module').then(m => m.RoomplanModule),
  },
  {
    path: 'Floorplanedit',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/floorplanedit/floorplan-edit.module').then(m => m.FloorplanEditModule),
  },
  {
    path: 'Timerplan',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/timerplan/timerplan.module').then(m => m.TimerplanModule),
  },
  {
    path: 'UserVariables',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/user-variables/user-variables.module').then(m => m.UserVariablesModule),
  },
  {
    path: 'Notification',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/send-notification/send-notification.module').then(m => m.SendNotificationModule),
  },
  {
    path: 'DPFibaro',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/dp/dp-fibaro/dp-fibaro.module').then(m => m.DpFibaroModule),
  },
  {
    path: 'DPHttp',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/dp/dp-http/dp-http.module').then(m => m.DpHttpModule),
  },
  {
    path: 'DPGooglePubSub',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/dp/dp-google-pub-sub/dp-google-pub-sub.module').then(m => m.DpGooglePubSubModule),
  },
  {
    path: 'DPInflux',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/dp/dp-influx/dp-influx.module').then(m => m.DpInfluxModule),
  },
  {
    path: 'Log',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/log/log.module').then(m => m.LogModule),
  },
  {
    path: 'RestoreDatabase',
    data: {permission: 'Admin'},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/restore-database/restore-database.module').then(m => m.RestoreDatabaseModule),
  },
  {
    path: 'secpanel',
    data: {permission: 'Admin', layout: false},
    canActivate: [OfflineGuard, AuthenticationGuard],
    loadChildren: () => import('./admin/security-panel/security-panel.module').then(m => m.SecurityPanelModule),
  },
  // Login/Logout & Offline (no guards)
  {
    path: 'Login',
    component: LoginComponent,
    canActivate: []
  },
  {
    path: 'Logout',
    component: LogoutComponent,
    canActivate: []
  },
  {
    path: 'Offline',
    component: OfflineComponent,
    canActivate: []
  },
  // If no route found, redirect to dashboard
  {
    path: 'mydomoticz/dashboard',
    redirectTo: '/Dashboard',
    pathMatch: 'full'
  },
  {
    path: '**',
    redirectTo: '/Dashboard',
    pathMatch: 'full'
  },
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule {
}
