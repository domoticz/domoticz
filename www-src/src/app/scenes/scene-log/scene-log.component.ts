import {Component, Inject, OnInit} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {SceneService} from 'src/app/_shared/_services/scene.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {SceneLog} from 'src/app/_shared/_models/scene';
import {LivesocketService} from '../../_shared/_services/livesocket.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-scene-log',
  templateUrl: './scene-log.component.html',
  styleUrls: ['./scene-log.component.css']
})
export class SceneLogComponent implements OnInit {

  sceneIdx: string;
  log: Array<SceneLog> = [];
  pageName: string;

  autoRefresh = true;

  constructor(
    private route: ActivatedRoute,
    private sceneService: SceneService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
    this.sceneIdx = this.route.snapshot.paramMap.get('idx');

    this.sceneService.getScene(this.sceneIdx).subscribe(scene => {
      this.pageName = scene.Name;

      this.refreshLog();
    });

    this.livesocketService.scene_update.subscribe(scene => {
      if (this.autoRefresh && scene.idx === this.sceneIdx) {
        this.refreshLog();
      }
    });
  }

  private refreshLog() {
    this.sceneService.getSceneLog(this.sceneIdx).subscribe(data => {
      this.log = data;
    });
  }

  clearLog() {
    bootbox.confirm(this.translationService.t('Are you sure to delete the Log?\n\nThis action can not be undone!'), (result) => {
      if (result !== true) {
        return;
      }

      this.sceneService.clearSceneLog(this.sceneIdx).subscribe(() => {
        this.refreshLog();
      }, error => {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Problem clearing the Log!'), 2500, true);
      });
    });
  }

}
