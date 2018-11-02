import { TestBed } from '@angular/core/testing';

import { DeviceLightService } from './device-light.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('DeviceLightService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: DeviceLightService = TestBed.get(DeviceLightService);
    expect(service).toBeTruthy();
  });
});
