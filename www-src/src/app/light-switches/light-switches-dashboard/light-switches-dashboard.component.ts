import {ChangeDetectorRef, Component, Inject, OnDestroy, OnInit} from '@angular/core';
import {ConfigService} from '../../_shared/_services/config.service';
import {DeviceService} from '../../_shared/_services/device.service';
import {empty, Subscription, timer} from 'rxjs';
import {TimesunService} from '../../_shared/_services/timesun.service';
import {DialogService} from '../../_shared/_services/dialog.service';
import {AddManualLightDeviceDialogComponent} from '../../_shared/_dialogs/add-manual-light-device-dialog/add-manual-light-device-dialog.component';
import {NotificationService} from '../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {catchError, delay, tap} from 'rxjs/operators';
import {AddLightDeviceDialogComponent} from '../../_shared/_dialogs/add-light-device-dialog/add-light-device-dialog.component';
import {ActivatedRoute} from '@angular/router';
import {LivesocketService} from '../../_shared/_services/livesocket.service';
import {Device, DevicesResponse} from '../../_shared/_models/device';

@Component({
  selector: 'dz-light-switches-dashboard',
  templateUrl: './light-switches-dashboard.component.html',
  styleUrls: ['./light-switches-dashboard.component.css']
})
export class LightSwitchesDashboardComponent implements OnInit, OnDestroy {

  items: Array<Device> = [];
  dragIdx: string;

  private broadcast_unsubscribe: Subscription = undefined;

  constructor(
    private configService: ConfigService,
    private deviceService: DeviceService,
    private timesunService: TimesunService,
    private dialogService: DialogService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private changeDetector: ChangeDetectorRef,
    private route: ActivatedRoute,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
    this.configService.LastUpdateTime = 0;

    this.broadcast_unsubscribe = this.livesocketService.device_update.subscribe(deviceData => {
      this.RefreshItem(deviceData);
    });

    this.ShowLights();
  }

  ngOnDestroy(): void {
    if (typeof this.broadcast_unsubscribe !== 'undefined') {
      this.broadcast_unsubscribe.unsubscribe();
      this.broadcast_unsubscribe = undefined;
    }
  }

  public onRoomChange(room: number) {
    this.ShowLights();
  }

  private ShowLights() {

    // $('#modal').show();

    const roomPlanId = this.route.snapshot.paramMap.get('room') ?
      Number(this.route.snapshot.paramMap.get('room')) : this.configService.globals.LastPlanSelected;

    this.deviceService.getDevices('light', true, '[Order]', undefined, roomPlanId).subscribe(data => {
      if (typeof data.result !== 'undefined') {
        if (typeof data.ActTime !== 'undefined') {
          this.configService.LastUpdateTime = Number(data.ActTime);
        }
        this.items = data.result;
      } else {
        this.items = [];
      }

      // $('#modal').hide();

      this.timesunService.RefreshTimeAndSun();

      // NB: had to do this because otherwise list of widgets not correctly updating after a drag/drop
      this.changeDetector.detectChanges();

      this.RefreshLights();
    });
  }

  private RefreshItem(newitem: Device): void {
    if (newitem !== null) {
      this.items.forEach((olditem, oldindex, oldarray) => {
        if (olditem.idx === newitem.idx) {
          oldarray[oldindex] = newitem;
        }
      });
    }
  }

  // We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
  private RefreshLights() {
    this.livesocketService.getJson<DevicesResponse>('json.htm?type=devices&filter=light&used=true&order=[Order]&lastupdate=' +
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
      this.ShowLights();
    });
  }

  public onReplaceDevice() {
    this.RefreshLights();
  }

  public onRemoveDevice(idx: string) {
    const indexToRemove = this.items.findIndex(t => {
      return t.idx === idx;
    });
    if (indexToRemove > -1) {
      this.items.splice(indexToRemove, 1);
    }
  }

  AddManualLightDevice() {
    const dialog = this.dialogService.addDialog(AddManualLightDeviceDialogComponent, {
      addCallback: () => {
        this.ShowLights();
      }
    }).instance;
    dialog.init();
    dialog.open();
  }

  AddLightDevice() {
    this.notificationService.ShowNotify(this.translationService.t('Press button on Remote...'));

    timer(600).subscribe(() => {
      let bHaveFoundDevice = false;
      let deviceID = '';
      let deviceidx = 0;
      let bIsUsed = false;
      let Name = '';

      this.deviceService.learnsw()
        .pipe(
          tap(data => {
            if (typeof data.status !== 'undefined') {
              if (data.status === 'OK') {
                bHaveFoundDevice = true;
                deviceID = data.ID;
                deviceidx = data.idx;
                bIsUsed = data.Used;
                Name = data.Name;
              }
            }
          }),
          catchError(() => {
            // Ignore error
            return empty();
          }),
          tap(() => {
            this.notificationService.HideNotify();
          }),
          delay(200)
        )
        .subscribe(() => {
          if ((bHaveFoundDevice === true) && (bIsUsed === false)) {
            const dialog = this.dialogService.addDialog(AddLightDeviceDialogComponent, {
              devIdx: deviceidx,
              addCallback: () => {
                this.ShowLights();
              }
            }).instance;
            dialog.init();
            dialog.open();
          } else {
            if (bIsUsed === true) {
              this.notificationService.ShowNotify(this.translationService.t('Already used by') + ':<br>"' + Name + '"', 3500, true);
            } else {
              this.notificationService.ShowNotify(this.translationService.t('Timeout...<br>Please try again!'), 2500, true);
            }
          }
        });
    });
  }

}
