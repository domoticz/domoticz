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
import {Plan} from '../../../_shared/_models/plan';
import {ConfigService} from '../../../_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {RoomService} from '../../../_shared/_services/room.service';
import {RoomPlanEditModalComponent} from '../room-plan-edit-modal/room-plan-edit-modal.component';

// FIXME proper import
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-room-plans-table',
  templateUrl: './room-plans-table.component.html',
  styleUrls: ['./room-plans-table.component.css']
})
export class RoomPlansTableComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() plans: Array<Plan>;

  @Output() update: EventEmitter<void> = new EventEmitter<void>();
  @Output() select: EventEmitter<Plan | null> = new EventEmitter<Plan | null>();

  @ViewChild('plantable', {static: false}) plantableRef: ElementRef;
  @ViewChild(RoomPlanEditModalComponent, {static: false}) editModal: RoomPlanEditModalComponent;

  private table: any = undefined;

  constructor(private configService: ConfigService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private roomService: RoomService) {
  }

  ngOnInit(): void {
  }

  ngAfterViewInit() {

    const orderRenderer = (value: string) => {
      const upIcon = '<button class="btn btn-icon js-move-up"><img src="../../images/up.png" /></button>';
      const downIcon = '<button class="btn btn-icon js-move-down"><img src="../../images/down.png" /></button>';
      const emptyIcon = '<img src="../../images/empty16.png" width="16" height="16" />';

      if (value === '1') {
        return downIcon + emptyIcon;
      } else if (parseInt(value, 10) === this.plans.length) {
        return emptyIcon + upIcon;
      } else {
        return downIcon + upIcon;
      }
    };

    const actionsRenderer = () => {
      const actions = [];
      actions.push('<button class="btn btn-icon js-rename" title="' + this.translationService.t('Rename') +
        '"><img src="../../images/rename.png" /></button>');
      actions.push('<button class="btn btn-icon js-remove" title="' + this.translationService.t('Remove') +
        '"><img src="../../images/delete.png" /></button>');
      return actions.join('&nbsp;');
    };

    this.table = $(this.plantableRef.nativeElement).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      order: [[2, 'asc']],
      ordering: false,
      columns: [
        {title: this.translationService.t('Idx'), width: '40px', data: 'idx'},
        {title: this.translationService.t('Name'), data: 'Name'},
        {
          title: this.translationService.t('Order'),
          className: 'actions-column',
          width: '50px',
          data: 'Order',
          render: orderRenderer
        },
        {title: '', className: 'actions-column', width: '40px', data: 'idx', render: actionsRenderer},
      ]
    }));

    const _this = this;

    _this.table.on('click', '.js-move-up', function () {
      const plan = _this.table.api().row($(this).closest('tr')).data() as Plan;
      _this.roomService.changePlanOrder(0, plan.idx).subscribe(() => {
        _this.update.emit();
      });
      return false;
    });

    _this.table.on('click', '.js-move-down', function () {
      const plan = _this.table.api().row($(this).closest('tr')).data() as Plan;
      _this.roomService.changePlanOrder(1, plan.idx).subscribe(() => {
        _this.update.emit();
      });
      return false;
    });

    _this.table.on('click', '.js-rename', function () {
      const plan = _this.table.api().row($(this).closest('tr')).data() as Plan;

      _this.editModal.open(plan.idx, plan.Name);
      _this.editModal.updated.subscribe(() => {
        _this.update.emit();
      });

      return false;
    });

    _this.table.on('click', '.js-remove', function () {
      const plan = _this.table.api().row($(this).closest('tr')).data() as Plan;

      bootbox.confirm(_this.translationService.t('Are you sure you want to delete this Plan?'), (result: boolean) => {
        if (result) {
          _this.roomService.removePlan(plan.idx).subscribe(() => {
            _this.update.emit();
          });
        }
      });

      return false;
    });

    _this.table.on('select.dt', function (event, row) {
      _this.select.emit(row.data() as Plan);
    });

    _this.table.on('deselect.dt', function () {
      // Timeout to prevent flickering when we select another item in the table
      // FIXME but without argument timeout = 0, so useless..? I removed the timeout.
      if (_this.table.api().rows({selected: true}).count() > 0) {
        return;
      }

      _this.select.emit(null);
    });
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (!this.table) {
      return;
    }

    if (changes.plans) {
      this.table.api().clear();
      this.table.api().rows
        .add(this.plans)
        .draw();
    }
  }

}
