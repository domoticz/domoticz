import { NotificationService } from '../../_services/notification.service';
import { Component, OnInit } from '@angular/core';
import {
  trigger,
  state,
  style,
  animate,
  transition,
} from '@angular/animations';

@Component({
  selector: 'dz-notification',
  templateUrl: './notification.component.html',
  styleUrls: ['./notification.component.css'],
  animations: [
    trigger('displayHide', [
      state('displayed', style({
        opacity: 1
      })),
      transition(':enter', [
        style({
          opacity: 0
        }),
        animate(400)
      ]),
      transition(':leave', animate(400, style({opacity: 0})))
    ])
  ]
})
export class NotificationComponent implements OnInit {

  display = false;

  notificationTxt: string;
  klass = '';

  constructor(private notificationService: NotificationService) { }

  ngOnInit() {
    this.notificationService.getNotification().subscribe(notif => {
      if (notif !== undefined) {
        // TODO animations fadeIn
        this.display = true;
        this.notificationTxt = notif.txt;
        this.klass = notif.iserror ? 'error' : '';
      } else {
        this.display = false;
        this.notificationTxt = '';
        this.klass = '';
      }
    });
  }

}
