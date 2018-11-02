import {TestBed} from '@angular/core/testing';

import {LocationService} from './location.service';
import {CommonTestModule} from '../_testing/common.test.module';
import {ForecastModule} from './forecast.module';

describe('LocationService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, ForecastModule]
  }));

  it('should be created', () => {
    const service: LocationService = TestBed.get(LocationService);
    expect(service).toBeTruthy();
  });
});
