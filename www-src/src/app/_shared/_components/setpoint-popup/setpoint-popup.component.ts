import { Component, OnInit, Inject, Output, EventEmitter } from '@angular/core';
import { SetpointService } from 'src/app/_shared/_services/setpoint.service';
import { NotificationService } from 'src/app/_shared/_services/notification.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { timer } from 'rxjs';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-setpoint-popup',
  templateUrl: './setpoint-popup.component.html',
  styleUrls: ['./setpoint-popup.component.css']
})
export class SetpointPopupComponent implements OnInit {

  @Output() set: EventEmitter<void> = new EventEmitter<void>();

  display = false;

  idx: string;

  // FIXME use integers
  curValue = '20.1';
  newValue = '';

  style: { [key: string]: string; } = {};

  constructor(
    private setpointService: SetpointService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) { }

  ngOnInit() {
  }

  public ShowSetpointPopup(event: any, idx: string, Protected: boolean, currentvalue: string, ismobile?: boolean) {
    event = event || window.event;
    // If pageX/Y aren't available and clientX/Y are,
    // calculate pageX/Y - logic taken from jQuery.
    // (This is to support old IE)
    if (event.pageX == null && event.clientX != null) {
      const eventDoc = (event.target && event.target.ownerDocument) || document;
      const doc = eventDoc.documentElement;
      const body = eventDoc.body;

      event.pageX = event.clientX +
        (doc && doc.scrollLeft || body && body.scrollLeft || 0) -
        (doc && doc.clientLeft || body && body.clientLeft || 0);
      event.pageY = event.clientY +
        (doc && doc.scrollTop || body && body.scrollTop || 0) -
        (doc && doc.clientTop || body && body.clientTop || 0);
    }
    const mouseX = event.pageX;
    const mouseY = event.pageY;

    this.ShowSetpointPopupInt(mouseX, mouseY, idx, currentvalue, ismobile);
  }

  private ShowSetpointPopupInt(mouseX: any, mouseY: any, idx: string, currentvalue: string, ismobile?: boolean) {
    this.idx = idx;

    this.curValue = parseFloat(currentvalue).toFixed(1);
    this.newValue = this.curValue;

    if (typeof ismobile === 'undefined') {
      this.style = {
        'top': mouseY,
        'left': mouseX + 15,
        'position': 'absolute',
        '-ms-transform': 'none',
        '-moz-transform': 'none',
        '-webkit-transform': 'none',
        'transform': 'none'
      };
    } else {
      this.style = {
        'position': 'fixed',
        'left': '50%',
        'top': '50%',
        '-ms-transform': 'translate(-50%,-50%)',
        '-moz-transform': 'translate(-50%,-50%)',
        '-webkit-transform': 'translate(-50%,-50%)',
        'transform': 'translate(-50%,-50%)'
      };
    }

    this.display = true;
  }

  public CloseSetpointPopup() {
    this.display = false;
  }

  public SetpointUp() {
    let curValue = parseFloat(this.newValue);
    curValue += 0.5;
    curValue = Math.round(curValue / 0.5) * 0.5;
    const curValueStr = curValue.toFixed(1);
    this.newValue = curValueStr;
  }

  public SetpointDown() {
    let curValue = parseFloat(this.newValue);
    curValue -= 0.5;
    curValue = Math.round(curValue / 0.5) * 0.5;
    if (curValue < 0) {
      curValue = 0;
    }
    const curValueStr = curValue.toFixed(1);
    this.newValue = curValueStr;
  }

  public SetSetpoint() {
    const curValue = parseFloat(this.newValue);
    this.setpointService.setSetpoint(this.idx, curValue).subscribe(data => {
      this.CloseSetpointPopup();
      if (data.status === 'ERROR') {
        this.notificationService.HideNotify();
        bootbox.alert(this.translationService.t('Problem setting Setpoint value'));
      }
      // wait 1 second
      timer(1000).subscribe(() => {
        this.notificationService.HideNotify();
        this.set.emit();
      });
    }, error => {
      this.notificationService.HideNotify();
      bootbox.alert(this.translationService.t('Problem setting Setpoint value'));
    });
  }

}
