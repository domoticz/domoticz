import { TestBed } from '@angular/core/testing';

import { MobileService } from './mobile.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {MobileModule} from "./mobile.module";

describe('MobileService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, MobileModule]
  }));

  it('should be created', () => {
    const service: MobileService = TestBed.get(MobileService);
    expect(service).toBeTruthy();
  });
});
