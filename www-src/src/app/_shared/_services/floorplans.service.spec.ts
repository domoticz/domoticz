import { TestBed } from '@angular/core/testing';

import { FloorplansService } from './floorplans.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('FloorplansService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: FloorplansService = TestBed.get(FloorplansService);
    expect(service).toBeTruthy();
  });
});
