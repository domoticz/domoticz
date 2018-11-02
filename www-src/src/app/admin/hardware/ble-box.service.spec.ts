import { TestBed } from '@angular/core/testing';

import { BleBoxService } from './ble-box.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {HardwareModule} from "./hardware.module";

describe('BleBoxService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HardwareModule]
  }));

  it('should be created', () => {
    const service: BleBoxService = TestBed.get(BleBoxService);
    expect(service).toBeTruthy();
  });
});
