import {Component, Inject, OnInit} from '@angular/core';
import {SecurityService} from './security.service';
import {interval, Observable, of, Subscription, timer} from 'rxjs';
import {HttpErrorResponse} from '@angular/common/http';
import {Router} from '@angular/router';
import {UserVariablesService} from '../../_shared/_services/user-variables.service';
import {Md5} from 'ts-md5/dist/md5';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {SetupService} from '../../_shared/_services/setup.service';
import {catchError, map} from 'rxjs/operators';

declare var ion: any;

@Component({
  selector: 'dz-security-panel',
  templateUrl: './security-panel.component.html',
  styleUrls: ['./security-panel.component.css']
})
export class SecurityPanelComponent implements OnInit {

  digitdisplay = '';
  password = '';

  RefreshTimer?: Subscription;
  CodeSetTimer?: Subscription;
  CountdownTimer?: Subscription;

  constructor(private securityService: SecurityService,
              private userVariablesService: UserVariablesService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private setupService: SetupService,
              private router: Router) {
  }

  ngOnInit() {
    ion.sound({
      sounds: [
        {name: 'arm'},
        {name: 'disarm'},
        {name: 'key'},
        {name: 'wrongcode'}
      ],

      // main config
      path: 'media/',
      preload: true,
      multiplay: true,
      volume: 0.9
    });

    this.ShowStatus();
    this.SetRefreshTimer();
  }

  ShowStatus() {
    this.securityService.getSecStatus().subscribe(data => {
      if (data.status !== 'OK') {
        this.ShowError('NoOkData');
      } else {
        let displaytext = '';
        if (data.secstatus === 0) {
          displaytext = 'DISARMED';
        } else if (data.secstatus === 1) {
          displaytext = 'ARM HOME';
        } else if (data.secstatus === 2) {
          displaytext = 'ARM AWAY';
        } else {
          displaytext = 'UNKNOWN';
        }
        this.digitdisplay = '* ' + displaytext + ' *';
      }
    }, (error: HttpErrorResponse) => {
      if (error.status === 401) { // 401 : Unauthorized
        // window.location = '/index.html';
        this.router.navigate(['/']);
      } else {
        this.ShowError('NoConnect');
      }
    });

    // if (1 == 2) {
    //   var elements = document.getElementsByClassName('digit');
    //   for (var i = 0; i < elements.length; i++) {
    //     elements[i].setAttribute('onClick', 'AddDigit(' + (i + 1) + ');');
    //   }
    //   document.getElementById('setall').setAttribute('onClick', 'beep();');
    // }

    this.password = '';
  }

  private ShowError(error: string): void {
    if (error === 'NoConnect') {
      this.digitdisplay = 'no connect';
    } else if (error === 'NoOkData') {
      this.digitdisplay = 'no ok data';
    } else {
      this.digitdisplay = 'unkown err';
    }
    this.RefreshTimer = timer(60000).subscribe(() => {
      this.SetRefreshTimer();
    });
  }

  private SetRefreshTimer(): void {
    if (typeof (this.RefreshTimer) !== 'undefined') {
      this.RefreshTimer.unsubscribe();
      this.RefreshTimer = undefined;
    }
    this.userVariablesService.getUserVariables().subscribe(data => {
      if (data.status !== 'OK') {
        this.ShowError('NoOkData');
      } else {
        if (typeof (data.result) !== 'undefined') {
          data.result.forEach((item, i) => {
            if (item.Name === 'secpanel-autorefresh') {
              this.RefreshTimer = timer(Number(item.Value) * 1000).subscribe(() => {
                this.ShowStatus();
                this.SetRefreshTimer();
              });
            } else {
              this.RefreshTimer = timer(60000).subscribe(() => {
                this.ShowStatus();
                this.SetRefreshTimer();
              });
            }
          });
        }
      }
    }, error => {
      this.ShowError('NoConnect');
    });
  }

  AddDigit(digit: string) {
    this.beep();
    if (typeof (this.CodeSetTimer) !== 'undefined') {
      this.CodeSetTimer.unsubscribe();
      this.CodeSetTimer = undefined;
    }
    if (typeof (this.RefreshTimer) !== 'undefined') {
      this.RefreshTimer.unsubscribe();
      this.RefreshTimer = undefined;
    }
    if (typeof (this.CountdownTimer) !== 'undefined') {
      this.CountdownTimer.unsubscribe();
      this.CountdownTimer = undefined;
      this.digitdisplay = '';
    }
    this.CodeSetTimer = timer(10000).subscribe(() => {
      this.ShowStatus();
      this.SetRefreshTimer();
    });

    let orgtext = this.password;
    if (isNaN(Number(orgtext))) {
      orgtext = '';
    }

    const newtext = orgtext + digit;
    let codeinput = '';
    for (let i = 0; i < newtext.length; i++) {
      codeinput = codeinput + '#';
    }

    this.digitdisplay = codeinput;
    this.password = newtext;
  }

  beep(tone?: string) {
    if (tone === 'error') {
      ion.sound.play('wrongcode');
    } else if (tone === 'set') {
      ion.sound.play('key');
    } else if (tone === 'in') {
      ion.sound.play('arm');
    } else if (tone === 'out') {
      ion.sound.play('disarm');
    } else {
      ion.sound.play('key');
    }
  }

  SetSecStatus(status: number) {
    if (typeof (this.CountdownTimer) !== 'undefined') {
      this.CountdownTimer.unsubscribe();
      this.CountdownTimer = undefined;
    }
    const seccode = this.password;
    if (isNaN(Number(seccode))) {
      this.beep('error');
      return;
    }
    if (typeof (this.RefreshTimer) !== 'undefined') {
      this.RefreshTimer.unsubscribe();
      this.RefreshTimer = undefined;
    }
    if (typeof (this.CodeSetTimer) !== 'undefined') {
      this.CodeSetTimer.unsubscribe();
      this.CodeSetTimer = undefined;
    }

    this.securityService.setSecStatus(status, Md5.hashStr(seccode) as string).subscribe(data => {
      if (data.status !== 'OK') {
        if (data.message === 'WRONG CODE') {
          this.digitdisplay = this.translationService.t('* WRONG CODE *');
          this.CodeSetTimer = timer(2000).subscribe(() => {
            this.ShowStatus();
            this.SetRefreshTimer();
          });
          this.password = '';
          this.beep('error');
        } else {
          this.ShowError('NoOkData');
        }
      } else {
        this.ShowStatus();
        if (status === 1 || status === 2) {
          this.ArmDelay().subscribe(timerz => {
            if (timerz > 0) {
              this.CountdownTimer = interval(1000).subscribe((i:number) => {
                this.countdown(timerz - i);
              });
              this.beep('set');
            } else {
              this.beep('in');
            }
          });
        } else {
          this.SetRefreshTimer();
          this.beep('out');
        }
      }
    }, error => {
      this.ShowError('NoConnect');
    });
  }

  private ArmDelay(): Observable<number | undefined> {
    let secondelay = 0;
    return this.setupService.getSettings().pipe(
      map(data => {
        if (data.status !== 'OK') {
          this.ShowError('NoOkData');
          return;
        } else {
          if (typeof (data.SecOnDelay) !== 'undefined') {
            secondelay = data.SecOnDelay;
          }
        }
        return secondelay;
      }),
      catchError((err) => {
        this.ShowError('NoConnect');
        return of(undefined);
      })
    );
  }

  private countdown(timerz: number) {
    if (timerz > 1) {
      // timerz = timerz - 1;
      this.beep('set');
      this.digitdisplay = 'Arm Delay: ' + timerz;
    } else {
      if (typeof (this.CountdownTimer) !== 'undefined') {
        this.CountdownTimer.unsubscribe();
        this.CountdownTimer = undefined;
      }
      this.beep('in');
      this.ShowStatus();
      this.SetRefreshTimer();
    }
  }

}
