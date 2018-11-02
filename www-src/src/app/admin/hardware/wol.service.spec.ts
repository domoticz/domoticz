import { TestBed } from '@angular/core/testing';

import { WolService } from './wol.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {HardwareModule} from "./hardware.module";

describe('WolService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HardwareModule]
  }));

  it('should be created', () => {
    const service: WolService = TestBed.get(WolService);
    expect(service).toBeTruthy();
  });
});
