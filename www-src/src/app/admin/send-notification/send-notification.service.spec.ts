import {TestBed} from '@angular/core/testing';

import {SendNotificationService} from './send-notification.service';
import {CommonTestModule} from '../../_testing/common.test.module';
import {SendNotificationModule} from "./send-notification.module";

describe('SendNotificationService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, SendNotificationModule]
  }));

  it('should be created', () => {
    const service: SendNotificationService = TestBed.get(SendNotificationService);
    expect(service).toBeTruthy();
  });
});
