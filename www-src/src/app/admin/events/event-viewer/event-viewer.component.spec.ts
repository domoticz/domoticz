import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {EventViewerComponent} from './event-viewer.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {ImportEventModalComponent} from '../import-event-modal/import-event-modal.component';
import {ExportEventModalComponent} from '../export-event-modal/export-event-modal.component';
import {EventUI} from '../events/events.component';
import {BlocklyXmlParserService} from '../blockly-xml-parser.service';
import {EventsModule} from "../events.module";

describe('EventViewerComponent', () => {
  let component: EventViewerComponent;
  let fixture: ComponentFixture<EventViewerComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, EventsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(EventViewerComponent);
    component = fixture.componentInstance;

    component.event = <EventUI>{
      eventstatus: '0',
      id: '1',
      name: 'Script #1',
      interpreter: 'Blockly'
    };

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
