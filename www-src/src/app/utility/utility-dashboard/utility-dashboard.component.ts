import {ChangeDetectorRef, Component, OnDestroy, OnInit} from '@angular/core';
import {Subscription} from 'rxjs';
import {TimesunService} from '../../_shared/_services/timesun.service';
import {DeviceService} from '../../_shared/_services/device.service';
import {ConfigService} from '../../_shared/_services/config.service';
import {ActivatedRoute} from '@angular/router';
import {LivesocketService} from '../../_shared/_services/livesocket.service';
import {Device, DevicesResponse} from '../../_shared/_models/device';

@Component({
  selector: 'dz-utility-dashboard',
  templateUrl: './utility-dashboard.component.html',
  styleUrls: ['./utility-dashboard.component.css']
})
export class UtilityDashboardComponent implements OnInit, OnDestroy {

  items: Array<Device> = [];

  private dragIdx: string;

  private broadcast_unsubscribe: Subscription = undefined;

  constructor(
    private configService: ConfigService,
    private deviceService: DeviceService,
    private timesunService: TimesunService,
    private changeDetector: ChangeDetectorRef,
    private route: ActivatedRoute,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
    this.broadcast_unsubscribe = this.livesocketService.device_update.subscribe((deviceData) => {
      this.RefreshItem(deviceData);
    });

    this.ShowUtilities();
  }

  ngOnDestroy(): void {
    if (typeof this.broadcast_unsubscribe !== 'undefined') {
      this.broadcast_unsubscribe.unsubscribe();
      this.broadcast_unsubscribe = undefined;
    }
  }

  public onRoomChange(room: number) {
    this.ShowUtilities();
  }

  private ShowUtilities() {

    // $('#modal').show();

    const roomPlanId = this.route.snapshot.paramMap.get('room') ?
      Number(this.route.snapshot.paramMap.get('room')) : this.configService.globals.LastPlanSelected;

    this.deviceService.getDevices('utility', true, '[Order]', undefined, roomPlanId)
      .subscribe(devices => {
        if (devices.result) {
          if (devices.ActTime) {
            this.configService.LastUpdateTime = Number(devices.ActTime);
          }
          this.items = devices.result;
        } else {
          this.items = [];
        }

        // NB: had to do this because otherwise list of widgets not correctly updating after a drag/drop
        this.changeDetector.detectChanges();

        // $('#modal').hide();

        this.timesunService.RefreshTimeAndSun();

        this.RefreshUtilities();
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
  private RefreshUtilities() {
    // this.deviceService.getDevices('utility', true, '[Order]', this.configService.LastUpdateTime,
    // this.configService.globals.LastPlanSelected)
    this.livesocketService.getJson<DevicesResponse>('json.htm?type=devices&filter=utility&used=true&order=[Order]&lastupdate=' +
      this.configService.LastUpdateTime + '&plan=' + this.configService.globals.LastPlanSelected, data => {
      if (typeof data.ServerTime !== 'undefined') {
        this.timesunService.SetTimeAndSun(data);
      }

      if (typeof data.result !== 'undefined') {
        if (typeof data.ActTime !== 'undefined') {
          this.configService.LastUpdateTime = Number(data.ActTime);
        }

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
    let roomid = this.configService.globals.LastPlanSelected;
    if (typeof roomid === 'undefined') {
      roomid = 0;
    }
    this.deviceService.switchDeviceOrder(myid, this.dragIdx, roomid).subscribe(response => {
      this.ShowUtilities();
    });
  }

  public onReplaceDevice() {
    this.RefreshUtilities();
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
