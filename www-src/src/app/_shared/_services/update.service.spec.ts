import {TestBed} from '@angular/core/testing';

import {UpdateService} from './update.service';
import {CommonTestModule} from "../../_testing/common.test.module";

describe('UpdateService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: UpdateService = TestBed.get(UpdateService);
    expect(service).toBeTruthy();
  });
});
