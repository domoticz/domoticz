import { ConfigService } from '../../_services/config.service';
import { Component, OnInit, Inject, ChangeDetectorRef, OnDestroy } from '@angular/core';
import { TimesunService } from 'src/app/_shared/_services/timesun.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { Subscription } from 'rxjs';

@Component({
  selector: 'dz-timesun',
  templateUrl: './timesun.component.html',
  styleUrls: ['./timesun.component.css']
})
export class TimesunComponent implements OnInit, OnDestroy {

  ServerTime = '';
  sunRise = '';
  sunSet = '';

  private subscription: Subscription;

  constructor(private configService: ConfigService,
    private timesunService: TimesunService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private changeDetector: ChangeDetectorRef) { }

  ngOnInit() {
    this.subscription = this.timesunService.getTimeAndSun().subscribe(data => {
      if (data) {
        this.sunRise = data.Sunrise;
        this.sunSet = data.Sunset;
        const month = data.ServerTime.split(' ')[0];
        this.ServerTime = data.ServerTime.replace(month, this.translationService.t(month));
      }
      // NB: had to do this because otherwise it was not updating the HTML after a drag/drop
      this.changeDetector.detectChanges();
    });
  }

  ngOnDestroy() {
    if (this.subscription) {
      this.subscription.unsubscribe();
    }
  }

  public isMobile(): boolean {
    return this.configService.globals.ismobile;
  }

}
