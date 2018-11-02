import { TestBed } from '@angular/core/testing';

import { LanguageService } from './language.service';
import { CommonTestModule } from '../../_testing/common.test.module';

describe('LanguageService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule]
  }));

  it('should be created', () => {
    const service: LanguageService = TestBed.get(LanguageService);
    expect(service).toBeTruthy();
  });
});
