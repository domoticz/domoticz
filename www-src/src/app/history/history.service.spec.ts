import {TestBed} from '@angular/core/testing';
import {HistoryService} from './history.service';
import {CommonTestModule} from '../_testing/common.test.module';
import {HistoryModule} from "./history.module";

describe('HistoryService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HistoryModule]
  }));

  it('should be created', () => {
    const service: HistoryService = TestBed.get(HistoryService);
    expect(service).toBeTruthy();
  });
});
