import { TestBed } from '@angular/core/testing';

import { ZwaveService } from './zwave.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {HardwareModule} from "./hardware.module";

describe('ZwaveService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HardwareModule]
  }));

  it('should be created', () => {
    const service: ZwaveService = TestBed.get(ZwaveService);
    expect(service).toBeTruthy();
  });
});
