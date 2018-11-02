import { TestBed } from '@angular/core/testing';

import { DeviceCounterReportDataService } from './device-counter-report-data.service';
import { CommonTestModule } from '../_testing/common.test.module';
import {DeviceReportModule} from "./device-report.module";

describe('DeviceCounterReportDataService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DeviceReportModule]
  }));

  it('should be created', () => {
    const service: DeviceCounterReportDataService = TestBed.get(DeviceCounterReportDataService);
    expect(service).toBeTruthy();
  });
});
