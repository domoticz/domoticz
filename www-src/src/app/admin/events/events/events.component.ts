import {Component, Inject, OnInit} from '@angular/core';
import {TimesunService} from '../../../_shared/_services/timesun.service';
import {EventsService} from '../events.service';
import {DzEvent, FetchEvents, FullDzEvent} from '../../../_shared/_models/events';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {tap} from 'rxjs/operators';
import {Observable} from 'rxjs';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-events',
  templateUrl: './events.component.html',
  styleUrls: ['./events.component.css']
})
export class EventsComponent implements OnInit {

  isListExpanded = true;

  search: { name: string } = {name: ''};

  activeEventId = 'states';

  events: Array<EventUI> = [];
  openedEvents: Array<EventUI> = [];

  luaTemplates: Array<TemplateUI> = [
    {id: 'All', name: 'All (commented)'},
    {id: 'Device', name: 'Device'},
    {id: 'Security', name: 'Security'},
    {id: 'Time', name: 'Time'},
    {id: 'UserVariable', name: 'User variable'}
  ];
  dzVentsTemplates: Array<TemplateUI> = [
    {id: 'All', name: 'All (commented)'},
    {id: 'Bare', name: 'Minimal'},
    {id: 'CustomEvents', name: 'Custom events'},
    {id: 'Device', name: 'Device'},
    {id: 'Group', name: 'Group'},
    {id: 'HTTPRequest', name: 'HTTP request'},
    {id: 'Scene', name: 'Scene'},
    {id: 'Security', name: 'Security'},
    {id: 'System', name: 'System events'},
    {id: 'Timer', name: 'Timer'},
    {id: 'UserVariable', name: 'User variable'},
    {id: 'global_data', name: 'Global Data'}
  ];

  private interpreters: Array<string> = [];

  constructor(private timesunService: TimesunService,
              private eventsService: EventsService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {
  }

  ngOnInit() {
    this.fetchEvents().subscribe(() => {
      // Nothing to do
    });
  }

  private fetchEvents(): Observable<FetchEvents> {
    this.timesunService.RefreshTimeAndSun();

    return this.eventsService.fetchEvents()
      .pipe(
        tap((data: FetchEvents) => {
          this.events = data.events.map((e: DzEvent) => {
            return <EventUI>{
              ...e,
              isChanged: false,
              isNew: false,
              interpreter: undefined,
              type: undefined,
              xmlstatement: undefined
            };
          });
          this.interpreters = data.interpreters;

          if (this.events.length > 0 && this.openedEvents.length === 0) {
            this.openEvent(this.events[0]);
          }
        })
      );
  }

  get eventsFiltered(): Array<EventUI> {
    return this.events.filter(e => {
      return e.name.includes(this.search.name);
    });
  }

  openEvent(event: EventUI): void {
    if (!this.openedEvents.find((item) => {
      return item.id === event.id;
    })) {
      this.openedEvents.push(event);
    }
    this.setActiveEventId(event.id);
  }

  setActiveEventId(eventId: string): void {
    this.activeEventId = eventId;
  }

  closeEvent(event: EventUI, forceClose: boolean = false) {

    const successFn = () => {
      if (this.activeEventId === event.id) {
        const index = this.openedEvents.indexOf(event);

        if (this.openedEvents[index + 1]) {
          this.openEvent(this.openedEvents[index + 1]);
        } else if (this.openedEvents[index - 1]) {
          this.openEvent(this.openedEvents[index - 1]);
        } else {
          this.activeEventId = 'states';
        }
      }

      this.openedEvents = this.openedEvents.filter((item) => {
        return item.id !== event.id;
      });

      event.isChanged = false;
    };

    if (event.isChanged && !forceClose) {
      bootbox.confirm(this.translationService.t('This script has unsaved changes.\n\nAre you sure you want to close it?'),
        (result: boolean) => {
          if (result === true) {
            successFn();
          }
        });
    } else {
      successFn();
    }
  }

  isInterpreterSupported(interpreter: string): boolean | undefined {
    if (!this.interpreters) {
      return undefined;
    }

    return this.interpreters.includes(interpreter);
  }

  createEvent(interpreter: string, eventtype?: string) {
    this.eventsService.getTemplate(interpreter, eventtype).subscribe((template) => {
      let index = 0;
      let name;

      do {
        index += 1;
        name = 'Script #' + index;
      } while (this.isNameExists(name));

      const event: EventUI = {
        id: name,
        eventstatus: '1',
        name: name,
        interpreter: interpreter,
        type: eventtype || 'All',
        xmlstatement: template,
        isChanged: true,
        isNew: true
      };

      this.openEvent(event);
    });
  }

  private isNameExists(name: string): boolean {
    return []
      .concat(this.events)
      .concat(this.openedEvents)
      .some((event: DzEvent) => {
        return event.name === name;
      });
  }

  deleteEvent(event: EventUI): void {
    this.events = this.events.filter((item) => {
      return item.id !== event.id;
    });

    this.closeEvent(event, true);
  }

  updateEvent(event: EventUI): void {
    if (event.isNew) {
      this.fetchEvents().subscribe(() => {
        this.openedEvents = this.openedEvents.map((item) => {
          if (item.id !== event.id) {
            return item;
          }

          const newEvent = this.events.find((ev) => {
            return ev.name === event.name;
          });

          if (this.activeEventId === event.id) {
            this.activeEventId = newEvent.id;
          }

          return newEvent;
        });
      });
    } else {
      const updates = {name: event.name, eventstatus: event.eventstatus};

      this.events = this.events.map((item) => {
        return item.id === event.id
          ? Object.assign({}, item, updates)
          : item;
      });

      this.openedEvents = this.openedEvents.map((item) => {
        return item.id === event.id
          ? Object.assign({}, item, updates)
          : item;
      });
    }
  }

  updateEventState(event: DzEvent): void {
    this.eventsService.updateEventState(event.id, event.eventstatus === '1').subscribe(() => {
      // Nothing to do
    });
  }

}

export interface EventUI extends FullDzEvent {
  isChanged: boolean;
  isNew: boolean;
}

interface TemplateUI {
  name: string;
  id: string;
}
