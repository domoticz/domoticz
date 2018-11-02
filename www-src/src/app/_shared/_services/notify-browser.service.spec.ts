import { TestBed } from '@angular/core/testing';

import { NotifyBrowserService } from './notify-browser.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('NotifyBrowserService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: NotifyBrowserService = TestBed.get(NotifyBrowserService);
    expect(service).toBeTruthy();
  });
});
