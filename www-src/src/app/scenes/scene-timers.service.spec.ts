import {TestBed} from '@angular/core/testing';

import {CommonTestModule} from '../_testing/common.test.module';
import {SceneTimersService} from './scene-timers.service';
import {ScenesModule} from './scenes.module';

describe('SceneTimersService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, ScenesModule]
  }));

  it('should be created', () => {
    const service: SceneTimersService = TestBed.get(SceneTimersService);
    expect(service).toBeTruthy();
  });
});
