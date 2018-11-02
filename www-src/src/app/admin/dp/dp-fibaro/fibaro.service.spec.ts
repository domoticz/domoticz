import {TestBed} from '@angular/core/testing';

import {FibaroService} from './fibaro.service';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {DpFibaroModule} from "./dp-fibaro.module";

describe('FibaroService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DpFibaroModule]
  }));

  it('should be created', () => {
    const service: FibaroService = TestBed.get(FibaroService);
    expect(service).toBeTruthy();
  });
});
