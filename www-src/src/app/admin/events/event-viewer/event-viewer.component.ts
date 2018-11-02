import {
  AfterViewInit, ChangeDetectorRef,
  Component,
  ElementRef,
  EventEmitter, HostListener, Inject,
  Input,
  OnDestroy,
  OnInit,
  Output,
  ViewChild
} from '@angular/core';
import {EventUI} from '../events/events.component';
import {EventsService} from '../events.service';
import {Observable, of, throwError, timer} from 'rxjs';
import {EventToUpdate, FullDzEvent} from '../../../_shared/_models/events';
import {ImportEventModalComponent} from '../import-event-modal/import-event-modal.component';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ExportEventModalComponent} from '../export-event-modal/export-event-modal.component';
import {mergeMap} from 'rxjs/operators';
import * as Blockly from 'blockly';
import {BlocklyXmlParserService} from '../blockly-xml-parser.service';
import {BlocklyOptions} from 'blockly';
import {DomoticzBlocklyToolbox} from '../blockly-toolbox';
import {BlocklyService} from '../blockly.service';
import * as ace from 'ace-builds';
// import 'ace-builds/src-noconflict/ace';
// import 'ace-builds/webpack-resolver';
import '../../../ace-webpack-resolver-custom';
import 'ace-builds/src-noconflict/ext-language_tools';
import 'ace-builds/src-noconflict/mode-python';
import 'ace-builds/src-noconflict/mode-lua';
import 'ace-builds/src-noconflict/theme-xcode';
// import 'ace-builds/src-noconflict/worker-lua';
import 'ace-builds/src-noconflict/snippets/lua';
import 'ace-builds/src-noconflict/snippets/python';
import 'ace-builds/src-noconflict/snippets/text';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-event-viewer',
  templateUrl: './event-viewer.component.html',
  styleUrls: ['./event-viewer.component.css']
})
export class EventViewerComponent implements OnInit, AfterViewInit, OnDestroy {

  @Input() event: EventUI;

  @Output() update: EventEmitter<EventUI> = new EventEmitter<EventUI>();
  @Output() delete: EventEmitter<EventUI> = new EventEmitter<EventUI>();

  eventData: FullDzEvent;

  eventTypes: Array<{ value: string; label: string }> = [
    {value: 'All', label: 'All'},
    {value: 'Device', label: 'Device'},
    {value: 'Security', label: 'Security'},
    {value: 'Time', label: 'Time'},
    {value: 'UserVariable', label: 'User variable'}
  ];

  @ViewChild('editor', { static: false }) editorRef: ElementRef;

  aceEditor: any;
  blocklyWorkspace: any;
  ro: any;

  @ViewChild(ImportEventModalComponent, { static: false }) importEventModal: ImportEventModalComponent;
  @ViewChild(ExportEventModalComponent, { static: false }) exportEventModal: ExportEventModalComponent;

  constructor(private eventsService: EventsService,
              private notificationService: NotificationService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private xmlParser: BlocklyXmlParserService,
              private blocklyService: BlocklyService,
              private changeDetectorRef: ChangeDetectorRef) {
  }

  ngOnInit() {
  }

  ngAfterViewInit(): void {
    const obs: Observable<FullDzEvent> = this.event.interpreter ?
      of(Object.assign({}, this.event)) :
      this.eventsService.fetchEvent(this.event.id);
    obs.subscribe((eventData) => {
      this.eventData = eventData;

      if (eventData.interpreter === 'Blockly') {
        this.initBlockly(eventData);
      } else {
        this.initAce(eventData);
      }

      this.changeDetectorRef.detectChanges();

    });
  }

  ngOnDestroy(): void {
    if (this.ro) {
      this.ro.disconnect();
    }
  }

  @HostListener('keydown', ['$event'])
  onKeyDown(e: KeyboardEvent): void {
    if ((e.ctrlKey || e.metaKey) && String.fromCharCode(e.which).toLowerCase() === 's') {

      if (this.event.isChanged) {
        this.saveEvent();
      }

      e.preventDefault();
      // return false;
    }
  }

  private initBlockly(eventData: FullDzEvent) {
    const _this = this;

    const container = this.editorRef.nativeElement;

    this.blocklyService.init().subscribe(() => {
      this.blocklyWorkspace = Blockly.inject(container, <BlocklyOptions>{
        path: './',
        toolbox: DomoticzBlocklyToolbox,
        zoom: {
          controls: true,
          wheel: true,
          startScale: 1.0,
          maxScale: 2,
          minScale: 0.3,
          scaleSpeed: 1.2
        },
        trashcan: true,
      });

      this.blocklyWorkspace.clear();

      if (eventData.xmlstatement) {
        const xml = Blockly.Xml.textToDom(eventData.xmlstatement);
        Blockly.Xml.domToWorkspace(xml, this.blocklyWorkspace);
      }

      $('body > .blocklyToolboxDiv').appendTo(container);

      // Timeout is required as Blockly fires change event right after it was initialized
      timer(200).subscribe(() => {
        this.blocklyWorkspace.addChangeListener(() => {
          _this.markEventAsUpdated();
        });
      });

      // Supported only in Chrome 67+
      // @ts-ignore
      if (window.ResizeObserver) {
        // @ts-ignore
        this.ro = new ResizeObserver((entries) => {
          if (entries.length && entries[0].contentRect.width > 0) {
            Blockly.svgResize(_this.blocklyWorkspace);
          }
        });

        this.ro.observe(container);
      }
    });
  }

  private initAce(eventData: FullDzEvent) {
    const _this = this;

    const element = this.editorRef.nativeElement;

    this.aceEditor = ace.edit(element);
    const interpreter = eventData.interpreter === 'dzVents' ? 'lua' : eventData.interpreter;

    this.aceEditor.setOptions({
      enableBasicAutocompletion: true,
      enableSnippets: true,
      enableLiveAutocompletion: true
    });

    this.aceEditor.setTheme('ace/theme/xcode');
    this.aceEditor.setValue(eventData.xmlstatement);
    this.aceEditor.getSession().setMode('ace/mode/' + interpreter.toLowerCase());
    this.aceEditor.gotoLine(1);
    this.aceEditor.scrollToLine(1, true, true);

    this.aceEditor.on('change', function (event) {
      _this.markEventAsUpdated();
    });
  }

  setEventState(isEnabled: boolean) {
    const newState = isEnabled ? '1' : '0';

    if (this.eventData.eventstatus === newState) {
      return;
    }

    this.eventData.eventstatus = newState;
    this.event.isChanged = true;
  }

  markEventAsUpdated(): void {
    this.event.isChanged = true;
  }

  isTriggerAvailable(): boolean {
    return this.eventData && ['Blockly', 'Lua', 'Python'].includes(this.eventData.interpreter);
  }

  importEvent(): void {
    this.importEventModal.open();
  }

  onImportEvent(scriptData: string) {
    try {
      const xml = Blockly.Xml.textToDom(scriptData);
      Blockly.Xml.domToWorkspace(xml, this.blocklyWorkspace);

      this.markEventAsUpdated();
    } catch (e) {
      this.notificationService.ShowNotify(this.translationService.t('Error importing script: data is not valid'), 2500, true);
    }
  }

  exportEvent(): void {
    const xml = Blockly.Xml.workspaceToDom(this.blocklyWorkspace);

    const scriptData = Blockly.Xml.domToText(xml);

    this.exportEventModal.open(scriptData);
  }

  saveEvent(): void {
    const event: EventToUpdate = Object.assign({}, this.eventData, this.event.isNew ? {id: undefined} : {});

    if (event.interpreter === 'dzVents') {
      if (event.name.indexOf('.lua') >= 0) {
        return bootbox.alert('You cannot have .lua in the name.');
      }

      if ([
        'Device', 'Domoticz', 'dzVents', 'EventHelpers', 'HistoricalStorage',
        'persistence', 'Time', 'TimedCommand', 'Timer', 'Utils', 'Security',
        'lodash', 'JSON', 'Variable'
      ].includes(event.name)) {
        return bootbox.alert('You cannot use these names for your scripts: Device, Domoticz, dzVents, EventHelpers, ' +
          'HistoricalStorage, persistence, Time, TimedCommand, Utils or Variable. It interferes with the dzVents subsystem. ' +
          'Please rename your script.');
      }
    }

    let obs: Observable<EventToUpdate>;
    if (event.interpreter !== 'Blockly') {
      event.xmlstatement = this.aceEditor.getValue();
      obs = of(event);
    } else {
      try {
        const xml = Blockly.Xml.workspaceToDom(this.blocklyWorkspace);

        event.xmlstatement = Blockly.Xml.domToText(xml);
        event.logicarray = JSON.stringify(this.xmlParser.parseXml(xml));
        obs = of(event);
      } catch (e) {
        obs = throwError(e.message);
      }
    }

    obs.pipe(
      mergeMap(e => {
        return this.eventsService.updateEvent(e);
      }),
    ).subscribe(() => {
      this.event.isChanged = false;
      this.update.emit(
        Object.assign({}, this.event, {
          name: event.name,
          eventstatus: event.eventstatus
        })
      );
    }, error => {
      bootbox.alert(error.toString());
    });
  }

  deleteEvent(): void {
    const message = 'Are you sure to delete this Event?\n\nThis action can not be undone...';

    const successFn = () => {
      this.notificationService.ShowNotify(this.translationService.t('Script successfully removed'), 1500);
      this.delete.emit(this.event);
    };

    if (this.event.isNew) {
      successFn();
    } else {
      bootbox.confirm(message, (result: boolean) => {
        if (result === true) {
          this.eventsService.deleteEvent(this.event.id).subscribe(() => {
            successFn();
          });
        }
      });
    }
  }

}
