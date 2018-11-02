import {TestBed} from '@angular/core/testing';

import {SecurityService} from './security.service';
import {CommonTestModule} from "../../_testing/common.test.module";
import {SecurityPanelModule} from "./security-panel.module";

describe('SecurityService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, SecurityPanelModule]
  }));

  it('should be created', () => {
    const service: SecurityService = TestBed.get(SecurityService);
    expect(service).toBeTruthy();
  });
});
