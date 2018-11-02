import {TestBed} from '@angular/core/testing';

import {CommonTestModule} from '../../_testing/common.test.module';
import {ConfigService} from './config.service';

describe('ConfigService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: ConfigService = TestBed.get(ConfigService);
    expect(service).toBeTruthy();
  });
});
