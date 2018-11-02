import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {EventsRoutingModule} from './events-routing.module';
import {FormsModule} from '@angular/forms';
import {EventsComponent} from './events/events.component';
import {EventsCurrentStatesComponent} from './events-current-states/events-current-states.component';
import {EventViewerComponent} from './event-viewer/event-viewer.component';
import {ExportEventModalComponent} from './export-event-modal/export-event-modal.component';
import {ImportEventModalComponent} from './import-event-modal/import-event-modal.component';
import {BlocklyXmlParserService} from './blockly-xml-parser.service';
import {SharedModule} from '../../_shared/shared.module';
import {BlocklyService} from './blockly.service';
import {EventsService} from './events.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    EventsRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    EventsComponent,
    EventsCurrentStatesComponent,
    EventViewerComponent,
    ExportEventModalComponent,
    ImportEventModalComponent
  ],
  exports: [],
  providers: [
    BlocklyService,
    BlocklyXmlParserService,
    EventsService
  ]
})
export class EventsModule {
}
