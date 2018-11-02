import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { TestBed } from '@angular/core/testing';

import { AboutService } from './about.service';
import {AboutModule} from "./about.module";

describe('AboutService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, AboutModule]
  }));

  it('should be created', () => {
    const service: AboutService = TestBed.get(AboutService);
    expect(service).toBeTruthy();
  });
});
