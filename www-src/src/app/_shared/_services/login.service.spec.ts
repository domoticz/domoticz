import { TestBed } from '@angular/core/testing';

import { LoginService } from './login.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('LoginService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: LoginService = TestBed.get(LoginService);
    expect(service).toBeTruthy();
  });
});
