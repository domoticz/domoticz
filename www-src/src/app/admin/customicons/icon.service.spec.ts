import { TestBed } from '@angular/core/testing';

import { IconService } from './icon.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {CustomiconsModule} from "./customicons.module";

describe('IconService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, CustomiconsModule]
  }));

  it('should be created', () => {
    const service: IconService = TestBed.get(IconService);
    expect(service).toBeTruthy();
  });
});
