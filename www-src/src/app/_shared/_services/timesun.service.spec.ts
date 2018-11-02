import {TestBed} from '@angular/core/testing';
import {CommonTestModule} from '../../_testing/common.test.module';
import {TimesunService} from './timesun.service';

describe('TimesunService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: TimesunService = TestBed.get(TimesunService);
    expect(service).toBeTruthy();
  });
});
