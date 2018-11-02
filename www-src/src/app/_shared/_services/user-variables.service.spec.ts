import {TestBed} from '@angular/core/testing';

import {UserVariablesService} from './user-variables.service';
import {CommonTestModule} from '../../_testing/common.test.module';

describe('UserVariablesService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: UserVariablesService = TestBed.get(UserVariablesService);
    expect(service).toBeTruthy();
  });
});
