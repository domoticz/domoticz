import { TestBed } from '@angular/core/testing';

import { SetupService } from './setup.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('SetupService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: SetupService = TestBed.get(SetupService);
    expect(service).toBeTruthy();
  });
});
