import { TestBed } from '@angular/core/testing';

import { LivesocketService } from './livesocket.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('LivesocketService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: LivesocketService = TestBed.get(LivesocketService);
    expect(service).toBeTruthy();
  });
});
