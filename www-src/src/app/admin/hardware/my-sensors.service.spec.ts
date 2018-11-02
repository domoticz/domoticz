import { TestBed } from '@angular/core/testing';

import { MySensorsService } from './my-sensors.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {HardwareModule} from "./hardware.module";

describe('MySensorsService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HardwareModule]
  }));

  it('should be created', () => {
    const service: MySensorsService = TestBed.get(MySensorsService);
    expect(service).toBeTruthy();
  });
});
