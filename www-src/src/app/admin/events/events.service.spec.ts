import {TestBed} from '@angular/core/testing';

import {EventsService} from './events.service';
import {CommonTestModule} from '../../_testing/common.test.module';
import {EventsModule} from "./events.module";

describe('EventsService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, EventsModule]
  }));

  it('should be created', () => {
    const service: EventsService = TestBed.get(EventsService);
    expect(service).toBeTruthy();
  });
});
