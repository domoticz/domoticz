import {TestBed} from '@angular/core/testing';

import {CommonTestModule} from '../_testing/common.test.module';
import {RegularTimersService} from './regular-timers.service';
import {DeviceTimersModule} from './device-timers.module';


describe('RegularTimersService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DeviceTimersModule]
  }));

  it('should be created', () => {
    const service: RegularTimersService = TestBed.get(RegularTimersService);
    expect(service).toBeTruthy();
  });
});
