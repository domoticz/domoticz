import { TestBed } from '@angular/core/testing';

import { DeviceTimerOptionsService } from './device-timer-options.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('DeviceTimerOptionsService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: DeviceTimerOptionsService = TestBed.get(DeviceTimerOptionsService);
    expect(service).toBeTruthy();
  });
});
