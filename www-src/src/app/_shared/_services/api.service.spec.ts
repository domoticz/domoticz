import {TestBed} from '@angular/core/testing';

import {CommonTestModule} from '../../_testing/common.test.module';
import {ApiService} from './api.service';

describe('ApiService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: ApiService = TestBed.get(ApiService);
    expect(service).toBeTruthy();
  });
});
