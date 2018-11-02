import { ApiResponse } from './api';

export interface NotificationTypes extends ApiResponse {
    result: Array<NotificationType>;
}

export interface NotificationType {
    ptag: string;
    text: string;
    val: number;
}

export interface Notifications extends ApiResponse {
    result?: Array<DeviceNotification>;
    notifiers: Array<Notifier>;
}

export interface Notifier {
    description: string;
    name: string;
}

export interface DeviceNotification {
    idx?: number;
    Priority: number;
    CustomMessage: string;
    SendAlways: boolean;
    ActiveSystems: string;
    Params?: string;
  }
