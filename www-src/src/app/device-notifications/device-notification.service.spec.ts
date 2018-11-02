import { TestBed } from '@angular/core/testing';

import { DeviceNotificationService } from './device-notification.service';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { CommonTestModule } from '../_testing/common.test.module';
import {DeviceNotificationsModule} from "./device-notifications.module";

describe('DeviceNotificationService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [HttpClientTestingModule, CommonTestModule, DeviceNotificationsModule]
  }));

  it('should be created', () => {
    const service: DeviceNotificationService = TestBed.get(DeviceNotificationService);
    expect(service).toBeTruthy();
  });
});
