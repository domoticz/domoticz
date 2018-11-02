import { TestBed } from '@angular/core/testing';

import { UsersService } from './users.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('UsersService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: UsersService = TestBed.get(UsersService);
    expect(service).toBeTruthy();
  });
});
