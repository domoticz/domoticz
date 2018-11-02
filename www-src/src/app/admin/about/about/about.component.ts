import {ConfigService} from 'src/app/_shared/_services/config.service';
import {AfterViewInit, Component, ElementRef, Inject, OnDestroy, OnInit, ViewChild} from '@angular/core';
import {GlobalConfig} from 'src/app/_shared/_models/config';
import {interval, Subscription, timer} from 'rxjs';
import {AboutService} from 'src/app/admin/about/about.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';

@Component({
  selector: 'dz-about',
  templateUrl: './about.component.html',
  styleUrls: ['./about.component.css']
})
export class AboutComponent implements OnInit, AfterViewInit, OnDestroy {

  config: GlobalConfig;
  appversion: string;
  strupptime = '-';

  private timer: Subscription;

  @ViewChild('canvas', {static: false}) canvasRef: ElementRef;
  @ViewChild('canvas2', {static: false}) canvas2Ref: ElementRef;
  @ViewChild('canvas3', {static: false}) canvas3Ref: ElementRef;

  constructor(
    private configService: ConfigService,
    private aboutService: AboutService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  ngOnInit() {
    this.config = this.configService.config;
    this.appversion = this.configService.appversion.getValue();

    this.RefreshUptime();
  }

  ngAfterViewInit() {
    this.drawBackground();
  }

  ngOnDestroy() {
    if (this.timer) {
      this.timer.unsubscribe();
      this.timer = undefined;
    }
  }

  private RefreshUptime() {
    if (this.timer) {
      this.timer.unsubscribe();
      this.timer = undefined;
    }

    this.aboutService.getUpTime().subscribe(data => {
      if (typeof data.status !== 'undefined') {
        let szUpdate = '';
        if (data.days !== 0) {
          szUpdate += data.days + ' ' + this.translationService.t('Days') + ', ';
        }
        if (data.hours !== 0) {
          szUpdate += data.hours + ' ' + this.translationService.t('Hours') + ', ';
        }
        if (data.minutes !== 0) {
          szUpdate += data.minutes + ' ' + this.translationService.t('Minutes') + ', ';
        }
        szUpdate += data.seconds + ' ' + this.translationService.t('Seconds');
        this.strupptime = szUpdate;
        this.timer = interval(5000).subscribe(() => {
          this.RefreshUptime();
        });
      }
    });
  }

  private drawBackground() {
    // The canvas element we are drawing into.
    // Credits to http://www.professorcloud.com/
    const $canvas = this.canvasRef.nativeElement;
    const $canvas2 = this.canvas2Ref.nativeElement;
    const $canvas3 = this.canvas3Ref.nativeElement;
    const ctx2 = $canvas2.getContext('2d');
    const ctx = $canvas.getContext('2d');
    const w = $canvas.width;
    const h = $canvas.height;
    const img = new Image();

    // A puff.
    // tslint:disable:no-bitwise
    const Puff = class {

      private opacity;
      private sy = (Math.random() * 285) >> 0;
      private sx = (Math.random() * 285) >> 0;

      constructor(private p: number) {
      }

      move(_timeFac: number) {
        this.p = this.p + 0.3 * _timeFac;
        this.opacity = (Math.sin(this.p * 0.05) * 0.5);
        if (this.opacity < 0) {
          this.p = this.opacity = 0;
          this.sy = (Math.random() * 285) >> 0;
          this.sx = (Math.random() * 285) >> 0;
        }
        this.p = this.p;
        ctx.globalAlpha = this.opacity;
        ctx.drawImage($canvas3, this.sy + this.p, this.sy + this.p, 285 - (this.p * 2), 285 - (this.p * 2), 0, 0, w, h);
      }
    };

    const puffs = [];
    const sortPuff = (p1, p2) => p1.p - p2.p;
    puffs.push(new Puff(0));
    puffs.push(new Puff(20));
    puffs.push(new Puff(40));

    let newTime, oldTime = 0, timeFac;

    const loop = () => {
      newTime = new Date().getTime();
      if (oldTime === 0) {
        oldTime = newTime;
      }
      timeFac = (newTime - oldTime) * 0.1;
      if (timeFac > 3) {
        timeFac = 3;
      }
      oldTime = newTime;
      puffs.sort(sortPuff);

      for (let i = 0; i < puffs.length; i++) {
        puffs[i].move(timeFac);
      }
      ctx2.drawImage($canvas, 0, 0, 570, 570);
      timer(10).subscribe(() => {
        loop();
      });
    };

    // Turns out Chrome is much faster doing bitmap work if the bitmap is in an existing canvas rather
    // than an IMG, VIDEO etc. So draw the big nebula image into canvas3
    const ctx3 = $canvas3.getContext('2d');
    img.onload = () => {
      ctx3.drawImage(img, 0, 0, 570, 570);
      loop();
    };
    img.src = 'images/nebula.jpg';
  }

}
