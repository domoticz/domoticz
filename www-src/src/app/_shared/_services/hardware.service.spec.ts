import { TestBed } from '@angular/core/testing';

import { HardwareService } from './hardware.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('HardwareService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: HardwareService = TestBed.get(HardwareService);
    expect(service).toBeTruthy();
  });
});
