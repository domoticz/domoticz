import { TestBed } from '@angular/core/testing';

import { DomoticzDevicesService } from './domoticz-devices.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('DomoticzDevicesService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: DomoticzDevicesService = TestBed.get(DomoticzDevicesService);
    expect(service).toBeTruthy();
  });
});
