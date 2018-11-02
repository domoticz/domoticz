import { TestBed } from '@angular/core/testing';

import { DpGooglePubSubService } from './dp-google-pub-sub.service';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {DpGooglePubSubModule} from "./dp-google-pub-sub.module";

describe('DpGooglePubSubService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [CommonTestModule, DpGooglePubSubModule]
  }));

  it('should be created', () => {
    const service: DpGooglePubSubService = TestBed.get(DpGooglePubSubService);
    expect(service).toBeTruthy();
  });
});
