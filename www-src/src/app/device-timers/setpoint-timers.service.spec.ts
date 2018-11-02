import {TestBed} from '@angular/core/testing';

import {CommonTestModule} from '../_testing/common.test.module';
import {SetpointTimersService} from './setpoint-timers.service';
import {DeviceTimersModule} from './device-timers.module';

describe('SetpointTimersService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DeviceTimersModule]
  }));

  it('should be created', () => {
    const service: SetpointTimersService = TestBed.get(SetpointTimersService);
    expect(service).toBeTruthy();
  });
});
