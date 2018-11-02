import {TestBed} from '@angular/core/testing';

import {CommonTestModule} from '../../_testing/common.test.module';
import {RoomService} from './room.service';

describe('RoomService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: RoomService = TestBed.get(RoomService);
    expect(service).toBeTruthy();
  });
});
