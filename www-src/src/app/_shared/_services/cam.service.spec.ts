import { TestBed } from '@angular/core/testing';

import { CamService } from './cam.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('CamService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: CamService = TestBed.get(CamService);
    expect(service).toBeTruthy();
  });
});
