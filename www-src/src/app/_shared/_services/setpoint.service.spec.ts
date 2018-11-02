import { TestBed } from '@angular/core/testing';

import { SetpointService } from './setpoint.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('SetpointService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: SetpointService = TestBed.get(SetpointService);
    expect(service).toBeTruthy();
  });
});
