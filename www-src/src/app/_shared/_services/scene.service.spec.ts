import { TestBed } from '@angular/core/testing';

import { SceneService } from './scene.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('SceneService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: SceneService = TestBed.get(SceneService);
    expect(service).toBeTruthy();
  });
});
