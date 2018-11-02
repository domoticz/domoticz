import { TestBed } from '@angular/core/testing';

import { DeviceTimerConfigUtilsService } from './device-timer-config-utils.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('DeviceTimerConfigUtilsService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: DeviceTimerConfigUtilsService = TestBed.get(DeviceTimerConfigUtilsService);
    expect(service).toBeTruthy();
  });
});
