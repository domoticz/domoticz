import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {EventsService} from '../events.service';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-events-current-states',
  templateUrl: './events-current-states.component.html',
  styleUrls: ['./events-current-states.component.css']
})
export class EventsCurrentStatesComponent implements OnInit, AfterViewInit {

  @ViewChild('table', { static: false }) tableRef: ElementRef;
  private table: any;

  constructor(@Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private eventsService: EventsService,
              private configService: ConfigService) {
  }

  ngOnInit() {
  }

  ngAfterViewInit(): void {
    this.table = $(this.tableRef.nativeElement).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      'sDom': 't',
      paging: false,
      columns: [
        {title: this.translationService.t('Idx'), data: 'id', type: 'num', width: '10%'},
        {title: this.translationService.t('Last update'), data: 'lastupdate', type: 'date-us', width: '20%'},
        {title: this.translationService.t('Name'), data: 'name', width: '34%'},
        {title: this.translationService.t('State'), data: 'value', width: '18%'},
        {title: this.translationService.t('Value'), data: 'values', width: '18%'}
      ]
    })).api();

    this.refresh();
  }

  refresh(): void {
    this.eventsService.fetchCurrentStates().subscribe(states => {
      const filteredStates = states.filter((item) => {
        return item.name !== 'Unknown';
      });

      this.table.clear();
      this.table.rows
        .add(filteredStates)
        .draw();
    });
  }

}
