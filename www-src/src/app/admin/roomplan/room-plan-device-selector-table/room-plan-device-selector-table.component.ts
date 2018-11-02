import {
  AfterViewInit,
  Component,
  ElementRef,
  EventEmitter,
  Inject,
  Input,
  OnChanges,
  OnInit,
  Output,
  SimpleChanges,
  ViewChild
} from '@angular/core';
import {PlanDeviceItem} from '../room-plan-device-selector-modal/room-plan-device-selector-modal.component';
import {ConfigService} from '../../../_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';

// FIXME proper import
declare var $: any;

@Component({
  selector: 'dz-room-plan-device-selector-table',
  templateUrl: './room-plan-device-selector-table.component.html',
  styleUrls: ['./room-plan-device-selector-table.component.css']
})
export class RoomPlanDeviceSelectorTableComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() devices: Array<PlanDeviceItem> = [];

  @Output() select: EventEmitter<PlanDeviceItem | null> = new EventEmitter<PlanDeviceItem | null>();

  @ViewChild('selectorTable', {static: false}) selectorTableRef: ElementRef;

  private table: any = undefined;

  constructor(private configService: ConfigService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.table = $(this.selectorTableRef.nativeElement).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      dom: '<"H"lfrC>t',
      order: [[1, 'asc']],
      paging: false,
      columns: [
        {title: this.translationService.t('Idx'), width: '40px', data: 'idx'},
        {title: this.translationService.t('Name'), data: 'Name'},
      ]
    }));

    const _this = this;

    this.table.on('select.dt', function (e, dt, type, indexes) {
      const item = dt.rows(indexes).data()[0] as PlanDeviceItem;
      _this.select.emit(item);
    });

    this.table.on('deselect.dt', function () {
      _this.select.emit(null);
    });

    this.showDevices(this.devices);
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes.devices) {
      this.showDevices(this.devices);
    }
  }

  private showDevices(devices: Array<PlanDeviceItem>) {
    if (!this.table) {
      return;
    }

    if (devices) {
      this.table.api().clear();
      this.table.api().rows
        .add(devices)
        .draw();
    }
  }

}
