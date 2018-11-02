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
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {Router} from '@angular/router';
import {DeviceAddModalComponent} from '../device-add-modal/device-add-modal.component';
import {DeviceRenameModalComponent} from '../device-rename-modal/device-rename-modal.component';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {DeviceUtils} from '../../../_shared/_utils/device-utils';
import {ConfigService} from '../../../_shared/_services/config.service';
import {Device} from '../../../_shared/_models/device';
import {LivesocketService} from '../../../_shared/_services/livesocket.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-devices-table',
  templateUrl: './devices-table.component.html',
  styleUrls: ['./devices-table.component.css']
})
export class DevicesTableComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() devices: Device[] = [];

  @Output() update: EventEmitter<any> = new EventEmitter<any>();

  @ViewChild('table', {static: false}) tableRef: ElementRef;
  table: any;

  @ViewChild(DeviceAddModalComponent, {static: false}) addModal: DeviceAddModalComponent;
  @ViewChild(DeviceRenameModalComponent, {static: false}) renameModal: DeviceRenameModalComponent;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    private router: Router,
    private notificationService: NotificationService,
    private configService: ConfigService,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const tableOptions = Object.assign({}, this.configService.dataTableDefaultSettings, {
      select: {
        style: 'multi',
        className: 'row_selected',
        selector: '.js-select-row'
      },
      order: [[13, 'desc']],
      columns: [
        {
          title: this.renderSelectorTitle(),
          width: '16px',
          orderable: false,
          defaultContent: this.selectorRenderer()
        },
        {
          title: this.renderDeviceStateTitle(),
          width: '16px',
          data: 'idx',
          orderable: false,
          render: this.iconRenderer.bind(this)
        },
        {title: this.translationService.t('Idx'), width: '30px', data: 'idx'},
        {title: this.translationService.t('Hardware'), width: '100px', data: 'HardwareName'},
        {
          title: this.translationService.t('ID'),
          width: '70px',
          data: 'ID',
          render: this.idRenderer.bind(this)
        },
        {title: this.translationService.t('Unit'), width: '40px', data: 'Unit'},
        {title: this.translationService.t('Name'), width: '200px', data: 'Name'},
        {title: this.translationService.t('Type'), width: '110px', data: 'Type'},
        {title: this.translationService.t('SubType'), width: '110px', data: 'SubType'},
        {title: this.translationService.t('Data'), data: 'Data'},
        {title: this.renderSignalLevelTitle(), width: '30px', data: 'SignalLevel'},
        {
          title: this.renderBatteryLevelTitle(),
          width: '30px',
          type: 'number',
          data: 'BatteryLevel',
          render: this.batteryLevelRenderer.bind(this)
        },
        {
          title: '',
          className: 'actions-column',
          width: '80px',
          data: 'idx',
          orderable: false,
          render: this.actionsRenderer.bind(this)
        },
        {title: this.translationService.t('Last Seen'), width: '150px', data: 'LastUpdate', type: 'date-us'},
      ]
    });

    const table = $(this.tableRef.nativeElement).dataTable(tableOptions);
    this.table = table;

    const _this = this;

    table.on('click', '.js-include-device', function () {
      const row = table.api().row($(this).closest('tr')).data();

      _this.addModal.open(Object.assign({}, row));
      _this.addModal.added.subscribe(() => {
        _this.update.emit();
      });
    });

    table.on('click', '.js-exclude-device', function () {
      const row = table.api().row($(this).closest('tr')).data();

      bootbox.confirm('Are you sure to remove this Device from your used devices?', (result) => {
        if (result === true) {
          _this.deviceService.excludeDevice(row.idx).subscribe(() => {
            _this.update.emit();
          });
        }
      });

    });

    table.on('click', '.js-rename-device', function () {
      const row = table.api().row($(this).closest('tr')).data();

      _this.renameModal.open(Object.assign({}, row));
      _this.renameModal.renamed.subscribe(() => {
        _this.update.emit();
      });
    });

    table.on('click', '.js-show-log', function () {
      const device = table.api().row($(this).closest('tr')).data();

      _this.deviceService.openCustomLog(device);
    });

    table.on('click', '.js-remove-device', function () {
      const device = table.api().row($(this).closest('tr')).data();

      bootbox.confirm('Are you sure to delete this Device?\n\nThis action can not be undone...', (result) => {
        if (result === true) {
          _this.deviceService.removeDevice(device.idx, '', '').subscribe(() => {
            _this.update.emit();
          });
        }
      });
    });

    table.on('click', '.js-remove-selected', function () {
      const selected_items = [].map.call(table.api().rows({selected: true}).data(), function (item) {
        const obj = {
          idx: item.idx,
          type: item.Type
        };
        return obj;
      });

      if (selected_items.length === 0) {
        return bootbox.alert('No Items selected to Delete!');
      }

      const devices = [];
      const scenes = [];

      selected_items.forEach((item) => {
        if ((item.type !== 'Group') && (item.type !== 'Scene')) {
          devices.push(item.idx);
        } else {
          scenes.push(item.idx);
        }
      });

      bootbox.confirm(_this.translationService.t('Are you sure you want to delete the selected Devices?') +
        ' (' + (devices.length + scenes.length) + ')', (result) => {
        if (result === true) {
          _this.notificationService.ShowNotify(_this.translationService.t('Removing...'), 30000);
          if (devices.length > 0) {
            _this.deviceService.removeDeviceAndSubDevices(devices).subscribe(() => {
              _this.notificationService.HideNotify();
              bootbox.alert((devices.length + scenes.length) + ' ' + _this.translationService.t('Devices deleted.'));
              _this.update.emit();
            });
          }
          if (scenes.length > 0) {
            _this.deviceService.removeScene(scenes).subscribe(() => {
              _this.notificationService.HideNotify();
              bootbox.alert((devices.length + scenes.length) + ' ' + _this.translationService.t('Devices deleted.'));
              _this.update.emit();
            });
          }
        }
      });
    });

    table.on('click', '.js-toggle-state', function () {
      const device = table.api().row($(this).closest('tr')).data();
      device.toggle();
    });

    table.on('change', '.js-select-devices', function () {
      if (this.checked) {
        table.api().rows({page: 'current'}).select();
      } else {
        table.api().rows({page: 'current'}).deselect();
      }

      table.find('.js-select-row').attr('checked', this.checked);
    });

    table.on('select.dt', function () {
      _this.updateDeviceDeleteBtnState();
    });

    table.on('deselect.dt', function () {
      _this.updateDeviceDeleteBtnState();
    });

    _this.livesocketService.device_update.subscribe(deviceData => _this.updateItem(deviceData, table));
    _this.livesocketService.scene_update.subscribe(deviceData => _this.updateItem(deviceData, table));

    table.api().rows
      .add(_this.devices)
      .draw();

    _this.updateDeviceDeleteBtnState();

    // Fix links so that it uses Angular router
    DatatableHelper.fixAngularLinks('.ng-link', this.router);
  }

  private updateItem(deviceData, table) {
    table.api().rows.every(function () {
      const device = this.data();

      if (device.idx === deviceData.idx) {
        this.data(Object.assign(device, deviceData));
        table.find('.row_selected .js-select-row').prop('checked', true);
      }
    });
  }

  ngOnChanges(changes: SimpleChanges) {
    if (!this.table) {
      return;
    }

    if (changes.devices) {
      this.table.api().clear();
      this.table.api().rows
        .add(this.devices)
        .draw();

      // Fix links so that it uses Angular router
      DatatableHelper.fixAngularLinks('.ng-link', this.router);
    }
  }

  getSelectedRecordsCounts() {
    return this.table.api().rows({selected: true}).count();
  }

  updateDeviceDeleteBtnState() {
    if (this.getSelectedRecordsCounts() > 0) {
      this.table.find('.js-remove-selected').show();
    } else {
      this.table.find('.js-remove-selected').hide();
    }
  }

  selectorRenderer() {
    return '<input type="checkbox" class="noscheck js-select-row" />';
  }

  idRenderer(value, type, device: Device) {
    if (DeviceUtils.isScene(device)) {
      return '-';
    }
    let ID = device.ID;
    if (typeof (device.HardwareTypeVal) !== 'undefined' && device.HardwareTypeVal === 21) {
      if (device.ID.substr(-4, 2) === '00') {
        ID = device.ID.substr(1, device.ID.length - 2) + '<span class="ui-state-default">' + device.ID.substr(-2, 2) + '</span>';
      } else {
        ID = device.ID.substr(1, device.ID.length - 4) + '<span class="ui-state-default">' + device.ID.substr(-4, 2) + '</span>' +
          device.ID.substr(-2, 2);
      }
    }
    /*
    Not sure why this was used
        if (device.Type == "Lighting 1") {
          ID = String.fromCharCode(device.ID);
        }
    */
    return ID;
  }

  iconRenderer(value, type, device: Device) {
    const itemImage = '<img src="' + DeviceUtils.icon(device).getIcon() + '" width="16" height="16">';

    const isToggleAvailable =
      (['Light/Switch', 'Lighting 2'].includes(device.Type) && [0, 7, 9, 10].includes(device.SwitchTypeVal))
      || device.Type === 'Color Switch'
      || DeviceUtils.isScene(device);

    if (isToggleAvailable) {
      const title = DeviceUtils.isActive(device) ? this.translationService.t('Turn Off') : this.translationService.t('Turn On');
      return '<button class="btn btn-icon js-toggle-state" title="' + title + '">' + itemImage + '</button>';
    } else {
      return itemImage;
    }
  }

  actionsRenderer(value, type, device: Device) {
    const actions = [];
    const logLink = DeviceUtils.getLogLink(device);
    const isScene = DeviceUtils.isScene(device);

    if (isScene) {
      actions.push('<img src="images/empty16.png">');
    } else if (device.Used !== 0) {
      actions.push('<button class="btn btn-icon js-exclude-device" title="' + this.translationService.t('Set Unused') +
        '"><img src="images/remove.png" /></button>');
    } else {
      actions.push('<button class="btn btn-icon js-include-device" title="' + this.translationService.t('Add Device') +
        '"><img src="images/add.png" /></button>');
    }

    actions.push('<button class="btn btn-icon js-rename-device" title="' + this.translationService.t('Rename Device') +
      '"><img src="images/rename.png" /></button>');

    if (isScene) {
      actions.push('<a class="btn btn-icon ng-link" href="/Scenes/' + device.idx + '/Log" title="' + this.translationService.t('Log') +
        '"><img src="images/log.png" /></a>');
    } else if (logLink) {
      actions.push('<a class="btn btn-icon ng-link" href="' + logLink.join('/') + '" title="' + this.translationService.t('Log') +
        '"><img src="images/log.png" /></a>');
    } else {
      actions.push('<button class="btn btn-icon js-show-log" title="' + this.translationService.t('Log') +
        '"><img src="images/log.png" /></button>');
    }

    actions.push('<button class="btn btn-icon js-remove-device" title="' + this.translationService.t('Remove') +
      '"><img src="images/delete.png" /></button>');

    return actions.join('&nbsp;');
  }

  batteryLevelRenderer(value, type) {
    if (!['display', 'filter'].includes(type)) {
      return value;
    }

    if (value === 255) {
      return '-';
    }

    const className = value < 10 ? 'empty' : value < 40 ? 'half' : 'full';
    const width = Math.ceil(value * 14 / 100);
    const title = this.translationService.t('Battery level') + ': ' + value + '%';

    return '<div class="battery ' + className + '" style="width: ' + width + 'px" title="' + title + '"></div>';
  }

  renderBatteryLevelTitle() {
    return '<img src="images/battery.png" style="transform: rotate(180deg);" title="' + this.translationService.t('Battery Level') + '">';
  }

  renderSignalLevelTitle() {
    return '<img src="images/air_signal.png" title="' + this.translationService.t('RF Signal Level') + '">';
  }

  renderDeviceStateTitle() {
    return '<button class="btn btn-icon js-remove-selected" title="' + this.translationService.t('Delete selected device(s)') +
      '"><img src="images/delete.png" /></button>';
  }

  renderSelectorTitle() {
    return '<input class="noscheck js-select-devices" type="checkbox" />';
  }
}
