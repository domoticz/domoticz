import {TimesunService} from 'src/app/_shared/_services/timesun.service';
import {Subscription} from 'rxjs';
import {DeviceService} from '../../_shared/_services/device.service';
import {ConfigService} from '../../_shared/_services/config.service';
import {ChangeDetectorRef, Component, OnDestroy, OnInit} from '@angular/core';
import {TimeAndSun} from 'src/app/_shared/_models/sunriseset';
import {ActivatedRoute} from '@angular/router';
import {LivesocketService} from '../../_shared/_services/livesocket.service';
import {Device, DevicesResponse} from '../../_shared/_models/device';

@Component({
  selector: 'dz-temperature',
  templateUrl: './temperature.component.html',
  styleUrls: ['./temperature.component.css']
})
export class TemperatureComponent implements OnInit, OnDestroy {

  temperatures: Array<Device> = [];

  private dragIdx: string;

  private broadcast_unsubscribe: Subscription = undefined;

  constructor(public configService: ConfigService,
              private deviceService: DeviceService,
              private timesunService: TimesunService,
              private changeDetector: ChangeDetectorRef,
              private route: ActivatedRoute,
              private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
    this.configService.LastUpdateTime = 0;

    this.broadcast_unsubscribe = this.livesocketService.device_update.subscribe((deviceData) => {
      this.RefreshItem(deviceData);
    });

    this.ShowTemps();
  }

  ngOnDestroy(): void {
    if (typeof this.broadcast_unsubscribe !== 'undefined') {
      this.broadcast_unsubscribe.unsubscribe();
      this.broadcast_unsubscribe = undefined;
    }
  }

  public onReplaceDevice() {
    this.RefreshTemps();
  }

  public onRemoveDevice(idx: string) {
    const indexToRemove = this.temperatures.findIndex(t => {
      return t.idx === idx;
    });
    if (indexToRemove > -1) {
      this.temperatures.splice(indexToRemove, 1);
    }
  }

  public isNotMobile(): boolean {
    return this.configService.globals.ismobile === false;
  }

  public onRoomChange(room: number) {
    this.ShowTemps();
  }

  private ShowTemps(): void {
    const roomPlanId = this.route.snapshot.paramMap.get('room') ?
      Number(this.route.snapshot.paramMap.get('room')) : this.configService.globals.LastPlanSelected;

    this.deviceService.getTemperatures(roomPlanId)
      .subscribe(temperatures => {
        if (temperatures.result) {
          if (temperatures.ActTime) {
            this.configService.LastUpdateTime = Number(temperatures.ActTime);
          }
          this.temperatures = temperatures.result;
        } else {
          this.temperatures = [];
        }

        // NB: had to do this because otherwise list of widgets not correctly updating after a drag/drop
        this.changeDetector.detectChanges();

        this.timesunService.RefreshTimeAndSun();

        this.RefreshTemps();
      });
  }

  private RefreshItem(newitem: Device): void {
    if (newitem !== null) {
      this.temperatures.forEach((olditem: Device, oldindex: number, oldarray: Device[]) => {
        if (olditem.idx === newitem.idx) {
          oldarray[oldindex] = newitem;
          // if ($scope.config.ShowUpdatedEffect == true) {
          // 	$("#tempwidgets #" + newitem.idx + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
          // }
        }
      });
    }
  }

  // We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
  private RefreshTemps(): void {
    const lastupdate = this.configService.LastUpdateTime;
    const lastPlanSelected = this.configService.globals.LastPlanSelected;

    // this.deviceService.getTemperatures(lastPlanSelected, lastupdate)
    this.livesocketService.getJson<DevicesResponse>(
      `json.htm?type=devices&filter=temp&used=true&order=[Order]&lastupdate=${lastupdate}&plan=${lastPlanSelected}`, temps => {

        if (temps.ServerTime) {
          const timeAndSun: TimeAndSun = temps;
          this.timesunService.SetTimeAndSun(timeAndSun);
        }

        if (temps.result) {
          if (temps.ActTime) {
            this.configService.LastUpdateTime = Number(temps.ActTime);
          }

          /*
						Render all the widgets at once.
					*/
          temps.result.forEach((newitem: Device) => {
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
    let roomid = this.configService.globals.LastPlanSelected;
    if (typeof roomid === 'undefined') {
      roomid = 0;
    }
    this.deviceService.switchDeviceOrder(myid, this.dragIdx, roomid).subscribe(response => {
      this.ShowTemps();
    });
  }

}
