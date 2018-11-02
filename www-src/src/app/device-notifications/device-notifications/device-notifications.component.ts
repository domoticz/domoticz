import {DeviceNotification, Notifications, NotificationType, Notifier} from '../../_shared/_models/notification-type';
import {Component, Inject, OnInit} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {DeviceService} from '../../_shared/_services/device.service';
import {forkJoin, Observable} from 'rxjs';
import {DeviceNotificationService} from '../device-notification.service';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Device} from '../../_shared/_models/device';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-device-notifications',
  templateUrl: './device-notifications.component.html',
  styleUrls: ['./device-notifications.component.css']
})
export class DeviceNotificationsComponent implements OnInit {

  pageName = '';

  notification: DeviceNotification;

  deviceIdx: string;
  device: Device;

  notificationTypes: Array<NotificationType>;
  notifications: Array<DeviceNotification>;
  notifiers: Array<Notifier>;

  isLoaded = false;

  constructor(
    private route: ActivatedRoute,
    private deviceService: DeviceService,
    private deviceNotificationService: DeviceNotificationService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) { }

  ngOnInit() {
    this.notification = {
      Priority: 0,
      CustomMessage: '',
      SendAlways: false,
      ActiveSystems: ''
    };

    this.deviceIdx = this.route.snapshot.paramMap.get('idx');

    forkJoin(
      this.deviceService.getDeviceInfo(this.deviceIdx),
      this.deviceNotificationService.getNotificationTypes(this.deviceIdx),
      this.getNotifications()
    ).subscribe((values: [Device, NotificationType[], Notifications]) => {
      const device = values[0];
      const notificationTypes = values[1];
      const response = values[2];

      this.pageName = device.Name;
      this.device = device;

      this.notificationTypes = notificationTypes;
      this.notification.Params = this.notificationTypes[0].ptag;

      this.applyNotifications(response);

      this.isLoaded = true;
    });
  }

  refreshNotifications(): void {
    this.getNotifications().subscribe(response => this.applyNotifications(response));
  }

  private getNotifications(): Observable<Notifications> {
    return this.deviceNotificationService.getNotifications(this.deviceIdx);
  }

  private applyNotifications(response: Notifications): void {
    this.notifications = response.result || [];
    this.notifiers = response.notifiers;
  }

  updateNotification() {
    this.deviceNotificationService.updateNotification(this.deviceIdx, this.notification.idx, this.getNotificationData(this.notification))
      .subscribe(() => {
        this.refreshNotifications();
      });
  }

  private getNotificationData(notification: DeviceNotification): any {
    const params: string[] = notification.Params.split(';');

    const norificationType = this.notificationTypes.find((item: NotificationType) => {
      return item.ptag === params[0];
    });

    return {
      ttype: norificationType.val,
      twhen: params[1]
        ? Object.keys(this.deviceNotificationService.deviceNotificationsConstants.whenByConditionMap).indexOf(params[1])
        : 0,
      tvalue: params[2] || 0,
      // tmsg: encodeURIComponent(notification.CustomMessage),
      tmsg: notification.CustomMessage,
      tsystems: notification.ActiveSystems,
      tpriority: notification.Priority,
      tsendalways: notification.SendAlways,
      trecovery: params[3] && parseInt(params[3]) === 1
    };
  }

  deleteNotification() {
    bootbox.confirm(this.translationService.t('Are you sure to delete this notification?\n\nThis action can not be undone...'), result => {
      if (result) {
        this.deviceNotificationService.deleteNotification(this.notification.idx).subscribe(() => {
          this.refreshNotifications();
        });
      }
    });
  }

  clearNotifications() {
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL notifications?\n\nThis action can not be undone!!'), result => {
      if (result) {
        this.deviceNotificationService.clearNotifications(this.deviceIdx).subscribe(() => {
          this.refreshNotifications();
        });
      }
    });
  }

  addNotification() {
    this.deviceNotificationService.addNotification(this.deviceIdx, this.getNotificationData(this.notification)).subscribe(() => {
      this.refreshNotifications();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem adding notification!<br>Duplicate Value?'), 2500, true);
    });
  }

  selectNotification(value: DeviceNotification) {
    if (value) {
      this.notification = Object.assign({}, value);
    } else {
      // FIXME we probably want to clear the whole notification object..
      this.notification.idx = undefined;
    }
  }

}
