import { TestBed } from '@angular/core/testing';

import { DeviceEnergyMultiCounterReportDataService } from './device-energy-multi-counter-report-data.service';
import { CommonTestModule } from '../_testing/common.test.module';
import {DeviceReportModule} from "./device-report.module";

describe('DeviceEnergyMultiCounterReportDataService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DeviceReportModule]
  }));

  it('should be created', () => {
    const service: DeviceEnergyMultiCounterReportDataService = TestBed.get(DeviceEnergyMultiCounterReportDataService);
    expect(service).toBeTruthy();
  });
});
