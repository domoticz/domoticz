import { TestBed } from '@angular/core/testing';

import { KodiService } from './kodi.service';
import { CommonTestModule } from '../../_testing/common.test.module';
import {HardwareModule} from "./hardware.module";

describe('KodiService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, HardwareModule]
  }));

  it('should be created', () => {
    const service: KodiService = TestBed.get(KodiService);
    expect(service).toBeTruthy();
  });
});
