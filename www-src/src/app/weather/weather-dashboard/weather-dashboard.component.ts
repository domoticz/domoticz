import {ChangeDetectorRef, Component, OnDestroy, OnInit} from '@angular/core';
import {ConfigService} from '../../_shared/_services/config.service';
import {DeviceService} from '../../_shared/_services/device.service';
import {TimesunService} from '../../_shared/_services/timesun.service';
import {Subscription} from 'rxjs';
import {LivesocketService} from '../../_shared/_services/livesocket.service';
import {Device, DevicesResponse} from "../../_shared/_models/device";

@Component({
  selector: 'dz-weather-dashboard',
  templateUrl: './weather-dashboard.component.html',
  styleUrls: ['./weather-dashboard.component.css']
})
export class WeatherDashboardComponent implements OnInit, OnDestroy {

  items: Array<Device> = [];

  private dragIdx: string;

  private broadcast_unsubscribe: Subscription = undefined;

  constructor(
    public configService: ConfigService,
    private deviceService: DeviceService,
    private timesunService: TimesunService,
    private changeDetector: ChangeDetectorRef,
    private livesocketService: LivesocketService) {
  }

  ngOnInit() {
    this.configService.LastUpdateTime = 0;

    this.broadcast_unsubscribe = this.livesocketService.device_update.subscribe((deviceData) => {
      this.RefreshItem(deviceData);
    });

    this.ShowWeathers();
  }

  ngOnDestroy(): void {
    if (typeof this.broadcast_unsubscribe !== 'undefined') {
      this.broadcast_unsubscribe.unsubscribe();
      this.broadcast_unsubscribe = undefined;
    }
  }

  private ShowWeathers() {
    this.deviceService.getDevices('weather', true, '[Order]').subscribe(response => {
      if (response.result) {
        if (response.ActTime) {
          this.configService.LastUpdateTime = response.ActTime;
        }
        this.items = response.result;
      } else {
        this.items = [];
      }

      // NB: had to do this because otherwise list of widgets not correctly updating after a drag/drop
      this.changeDetector.detectChanges();

      this.timesunService.RefreshTimeAndSun();

      this.RefreshWeathers();
    });
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

  // We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
  private RefreshWeathers() {
    // this.deviceService.getDevices('weather', true, '[Order]', this.configService.LastUpdateTime)
    this.livesocketService.getJson<DevicesResponse>('json.htm?type=devices&filter=weather&used=true&order=[Order]&lastupdate=' +
      this.configService.LastUpdateTime + '&plan=' + this.configService.globals.LastPlanSelected, response => {
      if (response.ServerTime) {
        this.timesunService.SetTimeAndSun(response);
      }

      if (response.result) {
        if (response.ActTime) {
          this.configService.LastUpdateTime = response.ActTime;
        }

        /*
        Render all the widgets at once.
         */
        response.result.forEach((newitem: Device) => {
          this.RefreshItem(newitem);
        });
      }

      // NB; in AngularJS app, there was a call to  SwitchLayout("Forecast") right here but I don't get why so I left it out...
    });
  }

  public onDragWidget(idx: string) {
    this.dragIdx = idx;
  }

  public onDropWidget(idx: string) {
    const myid = idx;
    this.deviceService.switchDeviceOrder(myid, this.dragIdx).subscribe(response => {
      this.ShowWeathers();
    });
  }

  public onReplaceDevice() {
    this.RefreshWeathers();
  }

  public onRemoveDevice(idx: string) {
    const indexToRemove = this.items.findIndex(t => {
      return t.idx === idx;
    });
    if (indexToRemove > -1) {
      this.items.splice(indexToRemove, 1);
    }
  }

}
