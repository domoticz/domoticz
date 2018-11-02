import { TestBed } from '@angular/core/testing';

import { LogService } from './log.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {LogModule} from "./log.module";

describe('LogService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, LogModule]
  }));

  it('should be created', () => {
    const service: LogService = TestBed.get(LogService);
    expect(service).toBeTruthy();
  });
});
