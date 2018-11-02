import { Component, OnInit, AfterViewInit, ViewChild, ElementRef, Inject, Input } from '@angular/core';
import { ConfigService } from '../../_shared/_services/config.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { DeviceService } from '../../_shared/_services/device.service';
import { LightSwitchesService } from '../../_shared/_services/light-switches.service';
import { LightSwitch } from 'src/app/_shared/_models/lightswitches';
import { NotificationService } from '../../_shared/_services/notification.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-device-subdevices-editor',
  templateUrl: './device-subdevices-editor.component.html',
  styleUrls: ['./device-subdevices-editor.component.css']
})
export class DeviceSubdevicesEditorComponent implements OnInit, AfterViewInit {

  @Input() deviceIdx: string;

  @ViewChild('subdevicestable', { static: false }) subdevicestableRef: ElementRef;
  table: any;

  selectedSubDeviceIdx: string;
  lightsAndSwitches: Array<LightSwitch> = [];
  subDeviceIdx: string;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private lightSwitchesService: LightSwitchesService,
    private notificationService: NotificationService
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const _this = this;

    this.table = $(this.subdevicestableRef.nativeElement).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      sDom: '<"H"frC>t<"F"i>',
      columns: [
        { title: this.translationService.t('Name'), data: 'Name' }
      ],
      select: {
        className: 'row_selected',
        style: 'single'
      },
    }));

    this.table.on('select.dt', function (e, dt, type, indexes) {
      const item = dt.rows(indexes).data()[0];
      _this.selectedSubDeviceIdx = item.ID;
      // $scope.$apply();
    });

    // TODO: Add caching mechanism for this request
    this.lightSwitchesService.getlightswitches().subscribe(data => {
      this.lightsAndSwitches = (data.result || []);
      this.refreshTable();
    });
  }

  private refreshTable() {
    this.table.api().clear().draw();

    this.lightSwitchesService.getSubDevices(this.deviceIdx).subscribe(data => {
      this.table.api().rows.add(data.result || []).draw();
    });
  }

  addSubDevice() {
    this.lightSwitchesService.addSubDevice(this.deviceIdx, this.subDeviceIdx).subscribe(() => {
      this.refreshTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem adding Sub/Slave Device!'), 2500, true);
    });
  }

  deleteSubDevice() {
    bootbox.confirm(this.translationService.t('Are you sure to delete this Sub/Slave Device?\n\nThis action can not be undone...'),
      (result) => {
        if (result !== true) {
          return;
        }

        this.lightSwitchesService.deleteSubDevice(this.selectedSubDeviceIdx).subscribe(() => {
          this.refreshTable();
        });
      });
  }

  clearSubDevices() {
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Sub/Slave Devices?\n\nThis action can not be undone!'),
      (result) => {
        if (result !== true) {
          return;
        }

        this.lightSwitchesService.deleteAllSubDevices(this.deviceIdx).subscribe(() => {
          this.refreshTable();
        });
      });
  }

}
