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
import {PlanDevice} from '../../../_shared/_models/plan';
import {ConfigService} from '../../../_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {RoomService} from '../../../_shared/_services/room.service';

// FIXME proper import
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-room-plan-devices-table',
  templateUrl: './room-plan-devices-table.component.html',
  styleUrls: ['./room-plan-devices-table.component.css']
})
export class RoomPlanDevicesTableComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() planId: string;
  @Input() devices: Array<PlanDevice>;

  @Output() update: EventEmitter<void> = new EventEmitter<void>();

  @ViewChild('planDevicesTable', {static: false}) planDevicesTableRef: ElementRef;

  private table: any = undefined;

  constructor(private configService: ConfigService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private roomService: RoomService) {
  }

  ngOnInit() {
  }

  ngAfterViewInit(): void {

    const orderRenderer = (value, renderType, plan, record) => {
      const upIcon = '<button class="btn btn-icon js-move-up"><img src="../../images/up.png" /></button>';
      const downIcon = '<button class="btn btn-icon js-move-down"><img src="../../images/down.png" /></button>';
      const emptyIcon = '<img src="../../images/empty16.png" width="16" height="16" />';

      if (record.row === 0) {
        return downIcon + emptyIcon;
      } else if (record.row === this.devices.length - 1) {
        return emptyIcon + upIcon;
      } else {
        return downIcon + upIcon;
      }
    };

    const actionsRenderer = () => {
      const actions = [];
      actions.push('<button class="btn btn-icon js-remove" title="' + this.translationService.t('Remove') +
        '"><img src="../../images/delete.png" /></button>');
      return actions.join('&nbsp;');
    };

    this.table = $(this.planDevicesTableRef.nativeElement).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      order: [[2, 'asc']],
      ordering: false,
      columns: [
        {title: this.translationService.t('Idx'), width: '40px', data: 'devidx'},
        {title: this.translationService.t('Name'), data: 'Name'},
        {title: this.translationService.t('Order'), width: '50px', data: 'Order', render: orderRenderer},
        {title: '', className: 'actions-column', width: '40px', data: 'idx', render: actionsRenderer},
      ]
    }));

    const _this = this;

    _this.table.on('click', '.js-move-up', function () {
      const device = _this.table.api().row($(this).closest('tr')).data();
      _this.roomService.changeDeviceOrder(0, _this.planId, device.idx).subscribe(() => {
        _this.update.emit();
      });
      return false;
    });

    _this.table.on('click', '.js-move-down', function () {
      const device = _this.table.api().row($(this).closest('tr')).data();
      _this.roomService.changeDeviceOrder(1, _this.planId, device.idx).subscribe(() => {
        _this.update.emit();
      });
      return false;
    });

    _this.table.on('click', '.js-remove', function () {
      const device = _this.table.api().row($(this).closest('tr')).data();

      bootbox.confirm(_this.translationService.t('Are you sure to delete this Active Device?\n\nThis action can not be undone...?'),
        (result: boolean) => {
          if (result) {
            _this.roomService.removeDeviceFromPlan(device.idx).subscribe(() => {
              _this.update.emit();
            });
          }
        });

      return false;
    });
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (!this.table) {
      return;
    }

    if (changes.devices) {
      this.table.api().clear();
      this.table.api().rows
        .add(this.devices)
        .draw();
    }
  }

}
