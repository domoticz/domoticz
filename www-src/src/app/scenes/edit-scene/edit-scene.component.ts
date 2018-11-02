import {Component, Inject, OnInit, ViewChild} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {SceneService} from 'src/app/_shared/_services/scene.service';
import {LightSwitchesService} from 'src/app/_shared/_services/light-switches.service';
import {LightSwitch} from '../../_shared/_models/lightswitches';
import {Utils} from 'src/app/_shared/_utils/utils';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {EditSceneActivationTableComponent} from '../edit-scene-activation-table/edit-scene-activation-table.component';
import {EditSceneDevicesTableComponent} from '../edit-scene-devices-table/edit-scene-devices-table.component';
import {NotificationService} from '../../_shared/_services/notification.service';
import {of, timer} from 'rxjs';
import {delay, mergeMap, tap} from 'rxjs/operators';
import {DeviceService} from '../../_shared/_services/device.service';
import {Learnsw} from '../../_shared/_models/learn';
import {Device} from '../../_shared/_models/device';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-edit-scene',
  templateUrl: './edit-scene.component.html',
  styleUrls: ['./edit-scene.component.css']
})
export class EditSceneComponent implements OnInit {

  private SceneIdx: string;
  item: Device;

  lightSwitches: Array<LightSwitch> = [];

  selectedDevice: LightSwitch = null;
  selectedDeviceColor: any = null;
  rgbwLevel = 50;

  deviceUpdateDeleteClass = 'btnstyle3-dis';
  deleteDeviceId: string = null;
  updateDeviceIds: [string, string] = null;

  removeCodeClass = 'btnstyle3-dis';
  removeCodeValues: [string, string] = null;

  command: string;
  level: string;
  ondelaytime: string;
  offdelaytime: string;
  onaction: string;
  offaction: string;
  type: string;

  @ViewChild(EditSceneActivationTableComponent, {static: false}) activationTable: EditSceneActivationTableComponent;
  @ViewChild(EditSceneDevicesTableComponent, {static: false}) devicesTable: EditSceneDevicesTableComponent;

  constructor(
    private route: ActivatedRoute,
    private sceneService: SceneService,
    private lightSwitchesService: LightSwitchesService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private router: Router,
    private deviceService: DeviceService
  ) {
  }

  ngOnInit() {

    this.SceneIdx = this.route.snapshot.paramMap.get('idx');

    this.sceneService.getScene(this.SceneIdx).subscribe(device => {
      this.item = device;

      this.onaction = atob(this.item.OnAction);
      this.offaction = atob(this.item.OffAction);

      this.type = this.item.Type === 'Scene' ? '0' : '1';
    });

    this.lightSwitchesService.getlightswitches()
      .subscribe(response => {
        this.lightSwitches = response.result;
      });
  }

  get displayRGBWPicker(): boolean {
    return this.selectedDevice != null && Utils.isLED(this.selectedDevice.SubType);
  }

  get displayLevelDiv(): boolean {
    // TODO: Show level combo box also for LED
    return this.selectedDevice != null && this.selectedDevice.IsDimmer && !Utils.isLED(this.selectedDevice.SubType);
  }

  get selectedDeviceLevels(): string[] {
    return this.selectedDevice.DimmerLevels.split(',');
  }

  onDevicesTableSelect(data: any) {
    const idx = data['DT_RowId'];
    const devidx = data['RealIdx'];

    this.deleteDeviceId = idx;
    this.updateDeviceIds = [idx, devidx];

    // $.lampIdx = devidx;
    this.selectedDevice = this.lightSwitches.find(ls => ls.idx === devidx);
    if (this.item.Type === 'Scene') {
      this.command = data['Command'];
    } else {
      this.command = 'On';
    }

    const level = data['Level'];
    this.level = level;

    this.selectedDeviceColor = data['Color'];

    this.ondelaytime = data['OnDelay'];
    this.offdelaytime = data['OffDelay'];
  }

  onActivationTableSelect(data: any) {
    const idx = data['DT_RowId'];
    const code = data['code'];
    this.removeCodeValues = [idx, code];
  }

  SaveScene() {
    let bValid = true;
    bValid = bValid && Utils.checkLength(this.item.Name, 2, 100);

    const onaction = this.onaction;
    const offaction = this.offaction;

    if (onaction !== '') {
      if (
        (onaction.indexOf('http://') === 0) ||
        (onaction.indexOf('https://') === 0) ||
        (onaction.indexOf('script://') === 0)
      ) {
        if (Utils.checkLength(onaction, 10, 500) === false) {
          bootbox.alert(this.translationService.t('Invalid ON Action!'));
          return;
        }
      } else {
        bootbox.alert(this.translationService.t('Invalid ON Action!'));
        return;
      }
    }
    if (offaction !== '') {
      if (
        (offaction.indexOf('http://') === 0) ||
        (offaction.indexOf('https://') === 0) ||
        (offaction.indexOf('script://') === 0)
      ) {
        if (Utils.checkLength(offaction, 10, 500) === false) {
          bootbox.alert(this.translationService.t('Invalid Off Action!'));
          return;
        }
      } else {
        bootbox.alert(this.translationService.t('Invalid Off Action!'));
        return;
      }
    }

    if (bValid) {
      const SceneType = this.type;
      const bIsProtected = this.item.Protected;
      this.sceneService.updateScene(this.SceneIdx, SceneType, this.item.Name, this.item.Description,
        btoa(onaction), btoa(offaction), bIsProtected).subscribe(() => {
        this.router.navigate(['/Scenes']);
      });
    }
  }

  DeleteScene() {
    bootbox.confirm(this.translationService.t('Are you sure to remove this Scene?'), (result) => {
      if (result === true) {
        this.sceneService.deleteScene(this.SceneIdx).subscribe(() => {
          this.router.navigate(['/Scenes']);
        });
      }
    });
  }

  AddCode() {
    this.notificationService.ShowNotify(this.translationService.t('Press button on Remote...'));

    timer(600)
      .pipe(
        mergeMap(() => this.deviceService.learnsw()),
        tap(() => {
          this.notificationService.HideNotify();
        }),
        delay(200),
        mergeMap((data: Learnsw) => {
          let bHaveFoundDevice = false;
          let deviceidx = 0;
          let Cmd = 0;

          if (typeof data.status !== 'undefined') {
            if (data.status === 'OK') {
              bHaveFoundDevice = true;
              deviceidx = data.idx;
              Cmd = data.Cmd;
            }
          }

          if (bHaveFoundDevice === true) {
            return this.sceneService.addSceneCode(this.SceneIdx, deviceidx, Cmd);
          } else {
            this.notificationService.ShowNotify(this.translationService.t('Timeout...<br>Please try again!'), 2500, true);
            return of(null);
          }
        })
      )
      .subscribe((data) => {
        if (data != null) {
          this.activationTable.RefreshActivators();
        }
      });
  }

  ClearCodes() {
    const bValid = false;
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Devices?\n\nThis action can not be undone!'), (result) => {
      if (result === true) {
        this.sceneService.clearSceneCodes(this.SceneIdx).subscribe(() => {
          this.activationTable.RefreshActivators();
        });
      }
    });
  }

  ClearDevices() {
    const bValid = false;
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Devices?\n\nThis action can not be undone!'), (result) => {
      if (result === true) {
        this.sceneService.deleteAllSceneDevices(this.SceneIdx).subscribe(() => {
          this.devicesTable.RefreshDeviceTable();
        });
      }
    });
  }

  AddDevice() {
    if (typeof this.selectedDevice === 'undefined') {
      bootbox.alert(this.translationService.t('No Device Selected!'));
      return;
    }
    const DeviceIdx = this.selectedDevice.idx;

    const Command = this.command;

    let level = '100';
    let colorJSON = ''; // Empty string, intentionally illegal JSON
    if (Utils.isLED(this.selectedDevice.SubType)) {
      level = this.rgbwLevel.toString();
      colorJSON = this.selectedDeviceColor;
    } else {
      if (this.selectedDevice.IsDimmer === true) {
        level = this.level;
      }
    }
    const ondelay = this.ondelaytime;
    const offdelay = this.offdelaytime;

    this.sceneService.addSceneDevice(this.SceneIdx, this.item.Type === 'Scene', DeviceIdx, Command,
      level, colorJSON, ondelay, offdelay).subscribe((data) => {
      if (data.status === 'OK') {
        this.devicesTable.RefreshDeviceTable();
      } else {
        this.notificationService.ShowNotify(this.translationService.t('Problem adding Device!'), 2500, true);
      }
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('Problem adding Device!'), 2500, true);
    });
  }

  UpdateDevice() {
    if (this.updateDeviceIds != null) {
      const idx = this.updateDeviceIds[0];
      const devidx = this.updateDeviceIds[1];

      const DeviceIdx = this.selectedDevice.idx;
      if (typeof DeviceIdx === 'undefined') {
        bootbox.alert(this.translationService.t('No Device Selected!'));
        return;
      }
      if (DeviceIdx !== devidx) {
        bootbox.alert(this.translationService.t('Device change not allowed!'));
        return;
      }

      const Command = this.command;

      let level = '100';
      let colorJSON = ''; // Empty string, intentionally illegal JSON
      const ondelay = this.ondelaytime;
      const offdelay = this.offdelaytime;

      if (Utils.isLED(this.selectedDevice.SubType)) {
        level = this.rgbwLevel.toString();
        colorJSON = this.selectedDeviceColor;
      } else {
        if (this.selectedDevice.IsDimmer === true) {
          level = this.level;
        }
      }

      this.sceneService.updateSceneDevice(idx, this.item.Type === 'Scene', DeviceIdx,
        Command, level, colorJSON, ondelay, offdelay)
        .subscribe(data => {
          if (data.status === 'OK') {
            this.devicesTable.RefreshDeviceTable();
          } else {
            this.notificationService.ShowNotify(this.translationService.t('Problem updating Device!'), 2500, true);
          }
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem updating Device!'), 2500, true);
        });
    }
  }

  DeleteDevice() {
    if (this.deleteDeviceId != null) {
      const idx = this.deleteDeviceId;
      bootbox.confirm(this.translationService.t('Are you sure to delete this Device?\n\nThis action can not be undone...'), (result) => {
        if (result === true) {
          this.sceneService.deleteSceneDevice(idx).subscribe(() => {
            this.devicesTable.RefreshDeviceTable();
          });
        }
      });
    }
  }

  RemoveCode() {
    if (this.removeCodeValues != null) {
      const idx = this.removeCodeValues[0];
      const code = this.removeCodeValues[1];

      if (this.removeCodeClass.includes('disabled')) {
        return false;
      }
      bootbox.confirm(this.translationService.t('Are you sure to delete this Device?\n\nThis action can not be undone...'), (result) => {
        if (result === true) {
          this.sceneService.removeSceneCode(this.SceneIdx, idx, code).subscribe(() => {
            this.activationTable.RefreshActivators();
          });
        }
      });
    }
  }

}
