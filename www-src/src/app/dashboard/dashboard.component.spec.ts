import {DashboardMobileSecurityWidgetComponent} from './dashboard-mobile-security-widget/dashboard-mobile-security-widget.component';
import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DashboardComponent} from './dashboard.component';
import {CommonTestModule} from '../_testing/common.test.module';
import {DashboardUtilityWidgetComponent} from './dashboard-utility-widget/dashboard-utility-widget.component';
import {DashboardWeatherWidgetComponent} from './dashboard-weather-widget/dashboard-weather-widget.component';
import {DashboardSecurityWidgetComponent} from './dashboard-security-widget/dashboard-security-widget.component';
import {DashboardSceneWidgetComponent} from './dashboard-scene-widget/dashboard-scene-widget.component';
import {DashboardLightSwitchesWidgetComponent} from './dashboard-light-switches-widget/dashboard-light-switches-widget.component';
import {DashboardMobileUtilityWidgetComponent} from './dashboard-mobile-utility-widget/dashboard-mobile-utility-widget.component';
import {DashboardMobileWeatherWidgetComponent} from './dashboard-mobile-weather-widget/dashboard-mobile-weather-widget.component';
import {DashboardMobileTemperatureWidgetComponent} from './dashboard-mobile-temperature-widget/dashboard-mobile-temperature-widget.component';
import {DashboardTemperatureWidgetComponent} from './dashboard-temperature-widget/dashboard-temperature-widget.component';
import {DashboardMobileLightSwitchesWidgetComponent} from './dashboard-mobile-light-switches-widget/dashboard-mobile-light-switches-widget.component';
import {DashboardMobileSceneWidgetComponent} from './dashboard-mobile-scene-widget/dashboard-mobile-scene-widget.component';
import {FormsModule} from '@angular/forms';

describe('DashboardComponent', () => {
  let component: DashboardComponent;
  let fixture: ComponentFixture<DashboardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        DashboardComponent,
        DashboardUtilityWidgetComponent,
        DashboardWeatherWidgetComponent,
        DashboardSecurityWidgetComponent,
        DashboardSceneWidgetComponent,
        DashboardLightSwitchesWidgetComponent,
        DashboardTemperatureWidgetComponent,
        DashboardMobileUtilityWidgetComponent,
        DashboardMobileWeatherWidgetComponent,
        DashboardMobileTemperatureWidgetComponent,
        DashboardMobileLightSwitchesWidgetComponent,
        DashboardMobileSceneWidgetComponent,
        DashboardMobileSecurityWidgetComponent,
      ],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DashboardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
