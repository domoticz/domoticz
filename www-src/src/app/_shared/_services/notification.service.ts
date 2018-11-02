import { Notification } from '../_models/notification';
import { Observable, BehaviorSubject } from 'rxjs';
import { Injectable } from '@angular/core';
import { timer, Subject } from 'rxjs';

@Injectable()
export class NotificationService {

  private curentNotification: BehaviorSubject<Notification> = new BehaviorSubject<Notification>(undefined);

  txt: string;
  iserror: boolean;

  constructor() {
  }

  ShowNotify(txt: string, timeout?: number, iserror: boolean = false): void {
    this.curentNotification.next({
      txt: txt,
      iserror: iserror
    });

    if (timeout !== undefined) {
      timer(timeout).subscribe(_ => {
        this.HideNotify();
      });
    }
  }

  HideNotify() {
    this.curentNotification.next(undefined);
  }

  getNotification(): Observable<Notification> {
    return this.curentNotification.asObservable();
  }

}
