import {ChangeDetectorRef, Component, Inject, OnDestroy, OnInit} from '@angular/core';
import {ConfigService} from '../_shared/_services/config.service';
import {DeviceService} from '../_shared/_services/device.service';
import {ActivatedRoute, Router} from '@angular/router';
import {TimesunService} from '../_shared/_services/timesun.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {PermissionService} from '../_shared/_services/permission.service';
import {LivesocketService} from '../_shared/_services/livesocket.service';
import {Device, DevicesResponse} from '../_shared/_models/device';
import {Subscription} from "rxjs";

// FIXME proper declaration
declare var bootbox: any;
declare var $: any;

@Component({
  selector: 'dz-dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.scss']
})
export class DashboardComponent implements OnInit, OnDestroy {

  items: Array<Device> = [];

  rowItems = 3;
  dragIdx: string;

  private devicesubscription: Subscription;
  private scenesubscription: Subscription;

  constructor(
    public configService: ConfigService,
    private deviceService: DeviceService,
    private router: Router,
    private timesunService: TimesunService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private changeDetector: ChangeDetectorRef,
    private permissionService: PermissionService,
    private route: ActivatedRoute,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
    this.configService.LastUpdateTime = 0;
    this.ShowFavorites();

    this.devicesubscription = this.livesocketService.device_update.subscribe((deviceData) => {
      this.RefreshItem(deviceData);
    });

    this.scenesubscription = this.livesocketService.scene_update.subscribe((sceneData) => {
      this.RefreshItem(sceneData);
    });
  }

  ngOnDestroy(): void {
    this.devicesubscription.unsubscribe();
    this.scenesubscription.unsubscribe();
  }

  get isAdmin(): boolean {
    return this.permissionService.getRights() === 2;
  }

  get mobileview(): boolean {
    return (this.configService.config.DashboardType === 2) || (this.configService.globals.ismobile === true);
  }

  private ShowFavorites() {
    let bFavorites = 1;

    const roomPlanId = this.route.snapshot.paramMap.get('room') ?
      Number(this.route.snapshot.paramMap.get('room')) : this.configService.globals.LastPlanSelected;

    if (typeof roomPlanId !== 'undefined') {
      if (roomPlanId > 0) {
        bFavorites = 0;
      }
    }

    if (this.configService.config.DashboardType === 3) {
      this.router.navigate(['/Floorplans']);
      $('body').addClass('dashFloorplan');
      return;
    }

    $('body').addClass('3column');
    if (this.configService.config.DashboardType === 1) {
      this.rowItems = 4;
      $('body').removeClass('3column').addClass('4column');
    }
    if (this.mobileview) {
      this.rowItems = 1000;
      $('body').addClass('dashMobile');
    }

    $('body').removeClass();
    $('body').addClass('dashboard');


    this.deviceService.getDevices('all', true, '[Order]', undefined, roomPlanId, bFavorites)
      .subscribe(data => {

        if (typeof data.ActTime !== 'undefined') {
          this.configService.LastUpdateTime = data.ActTime;
        }

        this.timesunService.RefreshTimeAndSun();

        if (typeof data.result !== 'undefined') {
          this.items = data.result;
        }

        // NB: had to do this because otherwise list of widgets not correctly updating after a drag/drop
        this.changeDetector.detectChanges();

      });

    this.RefreshFavorites();
  }

  get temperatureDevices(): Array<Device> {
    return this.items.filter(item => {
      return ((typeof item.Temp !== 'undefined') || (typeof item.Humidity !== 'undefined') || (typeof item.Chill !== 'undefined'));
    });
  }

  get weatherDevices(): Array<Device> {
    return this.items.filter(item => {
      return ((typeof item.Rain !== 'undefined') || (typeof item.Visibility !== 'undefined') ||
        (typeof item.UVI !== 'undefined') || (typeof item.Radiation !== 'undefined') ||
        (typeof item.Direction !== 'undefined') || (typeof item.Barometer !== 'undefined'));
    });
  }

  get securityDevices(): Array<Device> {
    return this.items.filter(item => {
      return (item.Type.indexOf('Security') === 0);
    });
  }

  get utilityDevices(): Array<Device> {
    return this.items.filter(item => {
      return (
        (typeof item.Counter !== 'undefined') ||
        (item.Type === 'Current') ||
        (item.Type === 'Energy') ||
        (item.SubType === 'kWh') ||
        (item.Type === 'Current/Energy') ||
        (item.Type === 'Power') ||
        (item.Type === 'Air Quality') ||
        (item.Type === 'Lux') ||
        (item.Type === 'Weight') ||
        (item.Type === 'Usage') ||
        (item.SubType === 'Percentage') ||
        ((item.Type === 'Thermostat') && (item.SubType === 'SetPoint')) ||
        (item.SubType === 'Soil Moisture') ||
        (item.SubType === 'Leaf Wetness') ||
        (item.SubType === 'Voltage') ||
        (item.SubType === 'Distance') ||
        (item.SubType === 'Current') ||
        (item.SubType === 'Text') ||
        (item.SubType === 'Alert') ||
        (item.SubType === 'Pressure') ||
        (item.SubType === 'A/D') ||
        (item.SubType === 'Thermostat Mode') ||
        (item.SubType === 'Thermostat Fan Mode') ||
        (item.SubType === 'Fan') ||
        (item.SubType === 'Smartwares') ||
        (item.SubType === 'Waterflow') ||
        (item.SubType === 'Sound Level') ||
        (item.SubType === 'Custom Sensor')
      );
    });
  }

  get sceneDevices(): Array<Device> {
    return this.items.filter(item => {
      return (item.Type.indexOf('Scene') === 0) || (item.Type.indexOf('Group') === 0);
    });
  }

  get lightDevices(): Array<Device> {
    return this.items.filter(item => {
      return (
        (item.Type.indexOf('Light') === 0) ||
        (item.SubType === 'Smartwares Mode') ||
        (item.Type.indexOf('Blind') === 0) ||
        (item.Type.indexOf('Curtain') === 0) ||
        (item.Type.indexOf('Thermostat 2') === 0) ||
        (item.Type.indexOf('Thermostat 3') === 0) ||
        (item.Type.indexOf('Chime') === 0) ||
        (item.Type.indexOf('Color Switch') === 0) ||
        (item.Type.indexOf('RFY') === 0) ||
        (item.Type.indexOf('ASA') === 0) ||
        (item.SubType === 'Relay') ||
        ((typeof item.SubType !== 'undefined') && (item.SubType.indexOf('Itho') === 0)) ||
        ((typeof item.SubType !== 'undefined') && (item.SubType.indexOf('Lucci') === 0)) ||
        ((typeof item.SubType !== 'undefined') && (item.SubType.indexOf('Westinghouse') === 0)) ||
        ((item.Type.indexOf('Value') === 0) && (typeof item.SwitchType !== 'undefined'))
      );
    });
  }

  get totdevices(): number {
    return this.temperatureDevices.length + this.weatherDevices.length + this.securityDevices.length +
      this.utilityDevices.length + this.sceneDevices.length + this.lightDevices.length;
  }

  public onRoomChange(room: number) {
    this.ShowFavorites();
  }

  private RefreshItem(newitem: Device) {
    if (newitem !== null) {
      this.items.forEach((olditem, oldindex, oldarray) => {
        if (olditem.idx === newitem.idx) {
          oldarray[oldindex] = newitem;
        }
      });
    }
  }

  private RefreshFavorites() {

    let bFavorites = 1;
    if (typeof this.configService.globals.LastPlanSelected !== 'undefined') {
      if (this.configService.globals.LastPlanSelected > 0) {
        bFavorites = 0;
      }
    }

    this.livesocketService.getJson<DevicesResponse>('json.htm?type=devices&filter=all&used=true&favorite=' +
      bFavorites + '&order=[Order]&plan=' + this.configService.globals.LastPlanSelected +
      '&lastupdate=' + this.configService.LastUpdateTime, data => {
      if (typeof data.ServerTime !== 'undefined') {
        this.timesunService.SetTimeAndSun(data);
      }

      if (typeof data.result !== 'undefined') {
        if (typeof data.ActTime !== 'undefined') {
          this.configService.LastUpdateTime = Number(data.ActTime);
        }

        console.log(data);

        /*
          Render all the widgets at once.
        */
        data.result.forEach((newitem: Device) => {
          this.RefreshItem(newitem);
        });

      }
    });
  }

  public onDragWidget(idx: string) {
    this.dragIdx = idx;
  }

  public onDropWidget(idx: string) {
    const myid = idx;

    const parts1 = myid.split('_');
    const parts2 = this.dragIdx.split('_');

    if (parts1[0] !== parts2[0]) {
      bootbox.alert(this.translationService.t('Only possible between Sensors of the same kind!'));
      this.ShowFavorites();
    } else {
      let roomid = this.configService.globals.LastPlanSelected;
      if (typeof roomid === 'undefined') {
        roomid = 0;
      }
      this.deviceService.switchDeviceOrder(parts1[1], parts2[1], roomid).subscribe(response => {
        this.ShowFavorites();
      });
    }
  }

  get spanClass(): string {
    if (this.configService.config.DashboardType === 0) {
      return 'span4';
    } else if (this.configService.config.DashboardType === 1) {
      return 'span3';
    }
  }

}
