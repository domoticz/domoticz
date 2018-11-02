import {Component, Inject, OnInit} from '@angular/core';
import {NotificationService} from '../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {SendNotificationService} from './send-notification.service';

@Component({
  selector: 'dz-send-notification',
  templateUrl: './send-notification.component.html',
  styleUrls: ['./send-notification.component.css']
})
export class SendNotificationComponent implements OnInit {

  subject = '';
  body = '';

  constructor(private notificationService: NotificationService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private sendNotificationService: SendNotificationService) {
  }

  ngOnInit() {
  }

  SendNotification() {

    const subject = this.subject;
    const body = this.body.replace(/\n/gi, '<br>');

    if (subject === '' || body === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Subject/Message!'), 2500, true);
      return;
    }

    this.sendNotificationService.sendNotification(subject, body).subscribe(() => {
      this.notificationService.ShowNotify(this.translationService.t('Notification Send'), 2500);
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Sending Notification!'), 2500, true);
    });
  }

}
