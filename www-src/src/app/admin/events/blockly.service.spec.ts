import {TestBed} from '@angular/core/testing';

import {BlocklyService} from './blockly.service';
import {CommonTestModule} from '../../_testing/common.test.module';
import {EventsModule} from "./events.module";

describe('BlocklyService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, EventsModule]
  }));

  it('should be created', () => {
    const service: BlocklyService = TestBed.get(BlocklyService);
    expect(service).toBeTruthy();
  });
});
