import {TestBed} from '@angular/core/testing';

import {BlocklyXmlParserService} from './blockly-xml-parser.service';
import {EventsModule} from './events.module';

describe('BlocklyXmlParserService', () => {
  beforeEach(() => TestBed.configureTestingModule({
    imports: [EventsModule],
    providers: []
  }));

  it('should be created', () => {
    const service: BlocklyXmlParserService = TestBed.get(BlocklyXmlParserService);
    expect(service).toBeTruthy();
  });
});
