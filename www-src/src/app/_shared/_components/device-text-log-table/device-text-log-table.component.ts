import {
  AfterViewInit,
  Component,
  ElementRef,
  Inject,
  Input,
  OnChanges,
  OnInit,
  SimpleChanges,
  ViewChild
} from '@angular/core';
import {ConfigService} from 'src/app/_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-text-log-table',
  templateUrl: './device-text-log-table.component.html',
  styleUrls: ['./device-text-log-table.component.css']
})
export class DeviceTextLogTableComponent implements OnInit, OnChanges, AfterViewInit {

  @Input() log: any; // FIXME type

  @ViewChild('table', {static: false}) private tableRef: ElementRef;

  table: any;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  ngOnInit() {
  }

  ngOnChanges(changes: SimpleChanges) {
    if (!this.table) {
      return;
    }

    if (changes.log) {
      this.table.clear();
      this.table.rows.add(this.log).draw();
    }
  }

  ngAfterViewInit() {
    this.table = $(this.tableRef.nativeElement).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      columns: [
        {title: this.translationService.t('Date'), data: 'Date', type: 'date-us'},
        {title: this.translationService.t('Data'), data: 'Data'},
        {title: this.translationService.t('User'), data: 'User', width: 140}
      ]
    })).api();
  }

}
