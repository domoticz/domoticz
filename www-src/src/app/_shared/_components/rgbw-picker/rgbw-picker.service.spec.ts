import { TestBed } from '@angular/core/testing';

import { RgbwPickerService } from './rgbw-picker.service';
import { CommonTestModule } from '../../../_testing/common.test.module';

describe('RgbwPickerService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: RgbwPickerService = TestBed.get(RgbwPickerService);
    expect(service).toBeTruthy();
  });
});
