import {Component, Input, OnInit} from '@angular/core';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {SceneService} from 'src/app/_shared/_services/scene.service';
import {mergeMap} from 'rxjs/operators';
import {Device} from '../../_shared/_models/device';

@Component({
  selector: '[dzDashboardMobileSceneWidget]',
  templateUrl: './dashboard-mobile-scene-widget.component.html',
  styleUrls: ['./dashboard-mobile-scene-widget.component.css']
})
export class DashboardMobileSceneWidgetComponent implements OnInit {

  @Input() item: Device;

  constructor(
    private sceneService: SceneService,
    private deviceService: DeviceService
  ) {
  }

  ngOnInit() {
  }

  SwitchScene() {
    this.sceneService.SwitchScene(this.item.idx, 'On', this.item.Protected).pipe(
      mergeMap(() => this.deviceService.getDeviceInfo(this.item.idx))
    ).subscribe(updatedItem => {
      this.item = updatedItem;
    });
  }

}
