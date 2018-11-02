import { TestBed } from '@angular/core/testing';

import { PingerService } from './pinger.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {HardwareModule} from "./hardware.module";

describe('PingerService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HardwareModule]
  }));

  it('should be created', () => {
    const service: PingerService = TestBed.get(PingerService);
    expect(service).toBeTruthy();
  });
});
