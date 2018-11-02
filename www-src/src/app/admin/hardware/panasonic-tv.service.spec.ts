import { TestBed } from '@angular/core/testing';

import { PanasonicTvService } from './panasonic-tv.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {HardwareModule} from "./hardware.module";

describe('PanasonicTvService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HardwareModule]
  }));

  it('should be created', () => {
    const service: PanasonicTvService = TestBed.get(PanasonicTvService);
    expect(service).toBeTruthy();
  });
});
