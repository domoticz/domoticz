import { TestBed } from '@angular/core/testing';

import { DeviceTemperatureReportDataService } from './device-temperature-report-data.service';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import {CommonTestModule} from '../_testing/common.test.module';
import {DeviceReportModule} from './device-report.module';

describe('DeviceTemperatureReportDataService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HttpClientTestingModule, DeviceReportModule]
  }));

  it('should be created', () => {
    const service: DeviceTemperatureReportDataService = TestBed.get(DeviceTemperatureReportDataService);
    expect(service).toBeTruthy();
  });
});
