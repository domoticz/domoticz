import { TestBed } from '@angular/core/testing';

import { ProtectionService } from './protection.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('ProtectionService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: ProtectionService = TestBed.get(ProtectionService);
    expect(service).toBeTruthy();
  });
});
