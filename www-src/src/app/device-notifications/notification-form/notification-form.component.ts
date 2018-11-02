import {Component, EventEmitter, Input, OnChanges, OnInit, Output, SimpleChanges} from '@angular/core';
import {DeviceNotification, NotificationType, Notifier} from '../../_shared/_models/notification-type';
import {DeviceNotificationService} from '../device-notification.service';
import {DeviceUtils} from '../../_shared/_utils/device-utils';
import {Device} from '../../_shared/_models/device';

@Component({
  selector: 'dz-notification-form',
  templateUrl: './notification-form.component.html',
  styleUrls: ['./notification-form.component.css']
})
export class NotificationFormComponent implements OnInit, OnChanges {

  @Input() device: Device;
  @Input() notificationTypes: Array<NotificationType>;
  @Input() notifiers: Array<Notifier>;

  @Input() notification: DeviceNotification;
  @Output() notificationChange = new EventEmitter<DeviceNotification>();

  notificationModel: NotificationFormModel;

  levelOptions?: Array<{ label: string, value: number }>;
  conditionOptions: { [condition: string]: string };
  priorityOptions: Array<string>;

  constructor(
    private deviceNotificationService: DeviceNotificationService) {
  }

  ngOnInit() {
    this.conditionOptions = this.deviceNotificationService.deviceNotificationsConstants.whenByConditionMap;
    this.priorityOptions = this.deviceNotificationService.deviceNotificationsConstants.priorities;

    if (DeviceUtils.isSelector(this.device)) {
      this.levelOptions = DeviceUtils.getSelectorLevelOptions(this.device);
    } else if (DeviceUtils.isDimmer(this.device)) {
      this.levelOptions = DeviceUtils.getDimmerLevelOptions(5);
    }
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.notification) {
      const activeSystems = this.notification.ActiveSystems ? this.notification.ActiveSystems.split(';') : [];

      const params = this.notification.Params.split(';');

      this.notificationModel = {
        type: params[0],
        condition: params[1],
        level: params[2] ? parseInt(params[2]) : undefined,
        priority: this.notification.Priority,
        customMessage: this.notification.CustomMessage,
        ignoreInterval: this.notification.SendAlways,
        sendRecovery: params[3] ? parseInt(params[3]) === 1 : false,
        systems: this.notifiers.reduce((acc: { [key: string]: boolean }, item: Notifier) => {
          acc[item.name] = activeSystems.length === 0 || activeSystems.includes(item.name);
          return acc;
        }, {})
      };
    }
  }

  updateViewValue() {
    const activeSystems = Object
      .keys(this.notificationModel.systems)
      .filter((key) => {
        return this.notificationModel.systems[key] === true;
      });

    const params = [this.notificationModel.type];

    if (this.isAdditionalOptionsAvailable()) {
      params.push(this.notificationModel.condition || '=');
      params.push(this.notificationModel.level.toString());
      params.push(this.notificationModel.sendRecovery ? '1' : '0');
    } else if (this.notificationModel.type === 'S' && this.notificationModel.level) {
      params.push(this.notificationModel.condition || '=');
      params.push(this.notificationModel.level.toString());
    }

    const value: DeviceNotification = Object.assign({}, this.notification, {
      Params: params.join(';'),
      Priority: this.notificationModel.priority,
      CustomMessage: this.notificationModel.customMessage,
      SendAlways: this.notificationModel.ignoreInterval,
      ActiveSystems: activeSystems.length === this.notifiers.length ? '' : activeSystems.join(';')
    });

    this.notificationChange.emit(value);
  }

  isAdditionalOptionsAvailable(): boolean {
    return this.deviceNotificationService.deviceNotificationsConstants.whenByTypeMap[this.notificationModel.type] === undefined;
  }

  getValueUnit(): string {
    return this.deviceNotificationService.deviceNotificationsConstants.unitByTypeMap[this.notificationModel.type] || '';
  }

}

class NotificationFormModel {
  type: string;
  condition: string;
  level: number;
  priority: number;
  customMessage: string;
  ignoreInterval: boolean;
  sendRecovery: boolean;
  systems: { [key: string]: boolean };
}

