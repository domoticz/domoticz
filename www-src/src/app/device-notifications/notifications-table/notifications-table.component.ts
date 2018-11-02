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
import {ConfigService} from '../../_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DeviceNotificationService} from '../device-notification.service';
import {DeviceNotification} from '../../_shared/_models/notification-type';
import {DeviceUtils} from '../../_shared/_utils/device-utils';
import {Device} from "../../_shared/_models/device";

// FIXME use proper ts def
declare var $: any;

@Component({
  selector: 'dz-notifications-table',
  templateUrl: './notifications-table.component.html',
  styleUrls: ['./notifications-table.component.css']
})
export class NotificationsTableComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() notifications: Array<DeviceNotification>;
  @Input() device: Device;

  @Output() select = new EventEmitter<DeviceNotification | null>();

  @ViewChild('table', {static: false}) table: ElementRef;

  datatable: any;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceNotificationService: DeviceNotificationService) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const tableElt = this.table.nativeElement;

    this.datatable = $(tableElt).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      columns: [
        {title: this.translationService.t('Type'), width: '120px', data: 'Params', render: (val) => this.typeRenderer(val)},
        {title: this.translationService.t('When'), width: '160px', data: 'Params', render: (val) => this.whenRenderer(val)},
        {
          title: this.translationService.t('Active Systems'),
          data: 'ActiveSystems',
          render: (val) => this.activeSystemsRenderer(val)
        },
        {
          title: this.translationService.t('Custom Message'),
          data: 'CustomMessage',
          render: (val) => this.customMessageRenderer(val)
        },
        {
          title: this.translationService.t('Priority'),
          width: '120px',
          data: 'Priority',
          render: (val) => this.priorityRenderer(val)
        },
        {
          title: this.translationService.t('Ignore Interval'),
          width: '120px',
          data: 'SendAlways',
          render: (val) => this.sendAlwaysRenderer(val)
        },
        {
          title: this.translationService.t('Recovery'),
          width: '80px',
          data: 'Params',
          render: (val) => this.recoveryRenderer(val)
        }
      ]
    })).api();

    this.datatable.on('select.dt', (e, dt, type, indexes) => {
      const item = dt.rows(indexes).data()[0];
      this.select.emit(item);
      // $scope.$apply();
    });

    this.datatable.on('deselect.dt', () => {
      this.select.emit(null);
      // $scope.$apply();
    });
  }

  ngOnChanges(changes: SimpleChanges) {
    if (!this.datatable) {
      return;
    }

    if (changes.notifications) {
      this.datatable.clear();
      this.datatable.rows
        .add(this.notifications)
        .draw();
    }
  }

  private typeRenderer(value: string) {
    const parts = value.split(';');
    const type = parts[0];

    return this.deviceNotificationService.deviceNotificationsConstants.typeNameByTypeMap[type] || '';
  }

  private whenRenderer(value: string) {
    const parts = value.split(';');
    const type = parts[0];
    const condition = parts[1] || '';
    const nvalue = Number(parts[2]);

    if (type === 'S' && DeviceUtils.isSelector(this.device)) {
      const levels = DeviceUtils.getLevels(this.device);
      return this.translationService.t('On') + ' (' + levels[nvalue / 10] + ')';
    } else if (this.deviceNotificationService.deviceNotificationsConstants.whenByTypeMap[type]) {
      return this.deviceNotificationService.deviceNotificationsConstants.whenByTypeMap[type];
    } else if (this.deviceNotificationService.deviceNotificationsConstants.whenByConditionMap[condition]) {
      const data = [
        this.deviceNotificationService.deviceNotificationsConstants.whenByConditionMap[condition]
      ];

      if (nvalue) {
        data.push(nvalue.toString());
      }

      if (this.deviceNotificationService.deviceNotificationsConstants.unitByTypeMap[type]) {
        data.push(this.deviceNotificationService.deviceNotificationsConstants.unitByTypeMap[type]);
      }

      return data.join('&nbsp;');
    }
  }

  private priorityRenderer(value: number) {
    return this.deviceNotificationService.deviceNotificationsConstants.priorities[value + 2];
  }

  private activeSystemsRenderer(value: string) {
    return value.length > 0 ? value : this.translationService.t('All');
  }

  private customMessageRenderer(value: string) {
    const parts = value.split(';;');
    return parts.length > 0 ? parts[0] : value;
  }

  private sendAlwaysRenderer(value: boolean) {
    return this.translationService.t(value === true ? 'Yes' : 'No');
  }

  private recoveryRenderer(value: string) {
    const parts = value.split(';');
    return this.translationService.t(parseInt(parts[3]) ? 'Yes' : 'No');
  }

}
