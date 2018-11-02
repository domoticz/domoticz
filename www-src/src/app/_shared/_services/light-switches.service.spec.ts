import { TestBed } from '@angular/core/testing';

import { LightSwitchesService } from './light-switches.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('LightSwitchesService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: LightSwitchesService = TestBed.get(LightSwitchesService);
    expect(service).toBeTruthy();
  });
});
