import {TestBed} from '@angular/core/testing';

import {InfluxService} from './influx.service';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {DpInfluxModule} from "./dp-influx.module";

describe('InfluxService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DpInfluxModule]
  }));

  it('should be created', () => {
    const service: InfluxService = TestBed.get(InfluxService);
    expect(service).toBeTruthy();
  });
});
