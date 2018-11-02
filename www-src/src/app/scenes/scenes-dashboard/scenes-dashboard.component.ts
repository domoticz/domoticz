import {ChangeDetectorRef, Component, OnDestroy, OnInit} from '@angular/core';
import {Subscription} from 'rxjs';
import {SceneService} from 'src/app/_shared/_services/scene.service';
import {ConfigService} from '../../_shared/_services/config.service';
import {TimesunService} from 'src/app/_shared/_services/timesun.service';
import {DialogService} from '../../_shared/_services/dialog.service';
import {AddSceneDialogComponent} from '../../_shared/_dialogs/add-scene-dialog/add-scene-dialog.component';
import {LivesocketService} from '../../_shared/_services/livesocket.service';
import {Device, DevicesResponse} from '../../_shared/_models/device';

@Component({
  selector: 'dz-scenes-dashboard',
  templateUrl: './scenes-dashboard.component.html',
  styleUrls: ['./scenes-dashboard.component.css']
})
export class ScenesDashboardComponent implements OnInit, OnDestroy {

  items: Array<Device> = [];

  private bAllowWidgetReorder = true;
  SceneIdx: string;

  private broadcast_unsubscribe: Subscription = undefined;

  constructor(
    private sceneService: SceneService,
    private configService: ConfigService,
    private timesunService: TimesunService,
    private changeDetector: ChangeDetectorRef,
    private dialogService: DialogService,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
    this.broadcast_unsubscribe = this.livesocketService.scene_update.subscribe((sceneData) => {
      this.RefreshItem(sceneData);
    });

    this.ShowScenes();
  }

  ngOnDestroy(): void {
    if (typeof this.broadcast_unsubscribe !== 'undefined') {
      this.broadcast_unsubscribe.unsubscribe();
      this.broadcast_unsubscribe = undefined;
    }
  }

  private ShowScenes() {
    this.sceneService.getScenes().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        if (typeof data.ActTime !== 'undefined') {
          this.configService.LastUpdateTime = data.ActTime;
        }

        this.items = data.result;

        this.bAllowWidgetReorder = data.AllowWidgetOrdering;

        // NB: had to do this because otherwise list of widgets not correctly updating after a drag/drop
        this.changeDetector.detectChanges();

        this.timesunService.RefreshTimeAndSun();

        this.RefreshScenes();
      }
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

  // We only call this once. After this the widgets are being updated automatically by used of the websocket broadcast event.
  private RefreshScenes() {
    this.livesocketService.getJson<DevicesResponse>('json.htm?type=scenes&lastupdate=' + this.configService.LastUpdateTime, response => {
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
    });
  }

  AddScene() {
    const dialog = this.dialogService.addDialog(AddSceneDialogComponent, {
      addedCallbackFn: () => {
        this.ShowScenes();
      }
    }).instance;
    dialog.init();
    dialog.open();
  }

  public onDragWidget(idx: string) {
    this.SceneIdx = idx;
  }

  public onDropWidget(idx: string) {
    const myid = idx;
    this.sceneService.switchSceneOrder(myid, this.SceneIdx).subscribe(response => {
      this.ShowScenes();
    });
  }

}
