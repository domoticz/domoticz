import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {InfluxService} from './influx.service';
import {DeviceService} from '../../../_shared/_services/device.service';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DatatableHelper} from '../../../_shared/_utils/datatable-helper';
import {Observable, of} from 'rxjs';
import {map} from 'rxjs/operators';
import {DeviceValueOption} from '../../../_shared/_models/device';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-dp-influx',
  templateUrl: './dp-influx.component.html',
  styleUrls: ['./dp-influx.component.css']
})
export class DpInfluxComponent implements OnInit, AfterViewInit {

  @ViewChild('iflinktable', {static: false}) iflinktableRef: ElementRef;

  devices: Array<{ name: string; value: string }> = [];

  linkToEdit: LinkToEdit = {
    idx: '0',
    linkenabled: true,
    targettype: '',
    sendvalue: '',
    devicename: ''
  };

  influxConfig: InfluxConfigUI = {
    database: '',
    debugenabled: false,
    linkenabled: false,
    password: '',
    path: '',
    tcpaddress: '',
    tcpport: '',
    username: ''
  };

  sendvalues: Array<DeviceValueOption> = [];

  constructor(private influxService: InfluxService,
              private deviceService: DeviceService,
              private notificationService: NotificationService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private configService: ConfigService) {
  }

  ngOnInit() {
    this.deviceService.listDevices().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.devices = data.result;
      }
      this.ValueSelectorUpdate();
    });

    this.LoadConfiguration();
  }

  ngAfterViewInit(): void {
    this.ShowLinks();
  }

  private LoadConfiguration() {
    this.influxService.getConfig().subscribe(data => {
      if (typeof data !== 'undefined') {
        if (data.status === 'OK') {
          this.influxConfig = {
            tcpaddress: data.InfluxIP,
            tcpport: data.InfluxPort,
            path: data.InfluxPath,
            database: data.InfluxDatabase,
            username: data.InfluxUsername,
            password: data.InfluxPassword,
            linkenabled: data.InfluxActive !== 0,
            debugenabled: data.InfluxDebug !== 0
          };
        }
      }
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem retrieving InfluxDB link settings!'), 2500, true);
    });
  }

  ShowLinks() {
    const _this = this;

    const oTable = $(this.iflinktableRef.nativeElement).dataTable({
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single'
      },
      'fnDrawCallback': function (oSettings) {
        const nTrs = this.fnGetNodes();
        $(nTrs).click(
          function () {
            $(nTrs).removeClass('row_selected');
            $(this).addClass('row_selected');
            $('#linkparamstable #linkupdate').attr('class', 'btnstyle3');
            $('#linkparamstable #linkdelete').attr('class', 'btnstyle3');
            const anSelected = DatatableHelper.fnGetSelected(oTable);
            if (anSelected.length !== 0) {
              const data = oTable.fnGetData(anSelected[0]) as DataRow;
              const idx = data['DT_RowId'];

              _this.linkToEdit = {
                idx: idx,
                targettype: data.TargetType,
                devicename: data.DeviceID,
                sendvalue: data.Delimitedvalue,
                linkenabled: data.Enabled === '1'
              };

              _this.ValueSelectorUpdate();
            }
          });
      },
      'aaSorting': [[0, 'desc']],
      'bSortClasses': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'aLengthMenu': [[15, 50, 100, -1], [15, 50, 100, 'All']],
      'iDisplayLength': 15,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });
    this.RefreshLinkTable();
  }

  RefreshLinkTable() {
    $('#linkparamstable #linkupdate').attr('class', 'btnstyle3-dis');
    $('#linkparamstable #linkdelete').attr('class', 'btnstyle3-dis');

    const oTable = $(this.iflinktableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.influxService.getInfluxLinks().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          let enabled = this.translationService.t('No');
          if (item.Enabled === '1') {
            enabled = this.translationService.t('Yes');
          }
          let DelimitedValueObs = of('');
          if (item.Delimitedvalue === '0') {
            DelimitedValueObs = of(this.translationService.t('Status'));
          } else {
            DelimitedValueObs = this.GetDeviceValueOptionWording(item.DeviceID, item.Delimitedvalue).pipe(
              map(wording => {
                return this.translationService.t(wording);
              })
            );
          }
          let TargetType = this.translationService.t('On Value Change');
          if (item.TargetType === '1') {
            TargetType = this.translationService.t('Direct');
          }

          DelimitedValueObs.subscribe(DelimitedValue => {
            oTable.fnAddData(<DataRow>{
              'DT_RowId': item.idx,
              'DeviceID': item.DeviceID,
              'TargetType': item.TargetType,
              'Enabled': item.Enabled,
              'Delimitedvalue': item.Delimitedvalue,
              '0': item.DeviceID,
              '1': item.Name,
              '2': DelimitedValue,
              '3': TargetType,
              '4': enabled
            });
          });

        });
      }
    });
  }

  private GetDeviceValueOptionWording(deviceid: string, pos: string): Observable<string> {
    return this.deviceService.getDeviceValueOptionWording(deviceid, pos).pipe(
      map(data => {
        if (typeof data.wording !== 'undefined') {
          return data.wording;
        } else {
          return '';
        }
      })
    );
  }

  ValueSelectorUpdate() {
    const deviceid = this.linkToEdit.devicename;
    this.sendvalues = [];
    this.deviceService.getDeviceValueOptions(deviceid).subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.sendvalues = data.result;
      }
    });
  }

  AddLink(type: string) {
    let idx = this.linkToEdit.idx;
    if (type === 'a') {
      idx = '0';
    }
    const deviceid = this.linkToEdit.devicename;
    const valuetosend = this.linkToEdit.sendvalue;
    const targettype = this.linkToEdit.targettype;
    let linkactive = 0;
    if (this.linkToEdit.linkenabled) {
      linkactive = 1;
    }

    this.influxService.saveInfluxLink(idx, deviceid, valuetosend, targettype, linkactive).subscribe(data => {
      bootbox.alert(this.translationService.t('InfluxDB link saved'));
      this.RefreshLinkTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving InfluxDB link!'), 2500, true);
    });
  }

  DeleteLink(idx: string) {
    bootbox.confirm(this.translationService.t('Are you sure you want to remove this link?'), (result) => {
      if (result === true) {
        this.influxService.deleteInfluxLink(idx).subscribe(data => {
          // bootbox.alert(this.translationService.t('Link deleted!'));
          this.RefreshLinkTable();
        });
      }
    });
  }

  SaveConfiguration() {
    let linkactive = 0;
    if (this.influxConfig.linkenabled) {
      linkactive = 1;
    }
    const remoteurl = this.influxConfig.tcpaddress;
    let port = this.influxConfig.tcpport;
    if (port.length === 0) {
      port = '8086';
    }
    const path = this.influxConfig.path;
    const database = this.influxConfig.database;
    const username = this.influxConfig.username;
    const password = this.influxConfig.password;
    let debugenabled = 0;
    if (this.influxConfig.debugenabled) {
      debugenabled = 1;
    }

    this.influxService.saveInfluxConfig(linkactive, remoteurl, port, path, database, username, password, debugenabled).subscribe(data => {
      bootbox.alert(this.translationService.t('InfluxDB link settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving InfluxDB link settings!'), 2500, true);
    });
  }

}

interface LinkToEdit {
  idx: string;
  targettype: string;
  devicename: string;
  sendvalue: string;
  linkenabled: boolean;
}

interface InfluxConfigUI {
  tcpaddress: string;
  tcpport: string;
  path: string;
  database: string;
  username: string;
  password: string;
  linkenabled: boolean;
  debugenabled: boolean;
}

interface DataRow {
  'DT_RowId': string;
  'DeviceID': string;
  'TargetType': string;
  'Enabled': string;
  'Delimitedvalue': string;
  '0': string;
  '1': string;
  '2': string;
  '3': string;
  '4': string;
}
