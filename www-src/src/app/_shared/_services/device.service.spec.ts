import {TestBed} from '@angular/core/testing';
import {CommonTestModule} from '../../_testing/common.test.module';
import {DeviceService} from './device.service';

describe('DeviceService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: DeviceService = TestBed.get(DeviceService);
    expect(service).toBeTruthy();
  });
});
