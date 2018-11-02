import {TestBed} from '@angular/core/testing';

import {DpHttpService} from './dp-http.service';
import {CommonTestModule} from "../../../_testing/common.test.module";
import {DpHttpModule} from "./dp-http.module";

describe('DpHttpService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DpHttpModule]
  }));

  it('should be created', () => {
    const service: DpHttpService = TestBed.get(DpHttpService);
    expect(service).toBeTruthy();
  });
});
