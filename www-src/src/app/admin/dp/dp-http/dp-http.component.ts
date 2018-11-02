import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {DeviceService} from '../../../_shared/_services/device.service';
import {DpHttpService} from './dp-http.service';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DatatableHelper} from '../../../_shared/_utils/datatable-helper';
import {Observable, of} from 'rxjs';
import {map} from 'rxjs/operators';
import {DeviceValueOption} from '../../../_shared/_models/device';
import {ConfigService} from "../../../_shared/_services/config.service";

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-dp-http',
  templateUrl: './dp-http.component.html',
  styleUrls: ['./dp-http.component.css']
})
export class DpHttpComponent implements OnInit, AfterViewInit {

  devices: Array<{ name: string; value: string }> = [];
  onOffDevices: Array<{ name: string; value: string; }> = [];

  httpConfig: HttpConfigUI = {
    debugenabled: false,
    linkenabled: false,
    headers: '',
    data: '',
    authBasicPassword: '',
    authBasicUser: '',
    auth: 0,
    method: 0,
    url: ''
  };

  @ViewChild('linkhttptable', {static: false}) linkhttptableRef: ElementRef;

  linkToEdit: LinkToEdit = {
    idx: '0',
    devicename: '',
    targetdeviceid: '',
    targetproperty: '',
    targetvariable: '',
    sendvaluereboot: '',
    sendvalue: '',
    sendvaluescene: '',
    includeunit: true,
    targettype: '0',
    onoffdevicename: '',
    linkenabled: true
  };
  sendvalues: Array<DeviceValueOption>;

  constructor(private deviceService: DeviceService,
              private httpService: DpHttpService,
              private notificationService: NotificationService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private configService: ConfigService) {
  }

  ngOnInit() {
    this.deviceService.listDevices().subscribe(data => {
      this.devices = data.result;
      this.ValueSelectorUpdate();
    });

    this.deviceService.listOnOffDevices().subscribe(data => {
      this.onOffDevices = data.result;
    });

    this.GetConfig();
  }

  ngAfterViewInit(): void {
    this.ShowLinks();
  }

  private GetConfig() {
    this.httpService.getConfig().subscribe(data => {
      if (typeof data !== 'undefined') {
        if (data.status === 'OK') {
          this.httpConfig = {
            url: data.HttpUrl,
            method: data.HttpMethod,
            data: data.HttpData,
            headers: data.HttpHeaders,
            linkenabled: data.HttpActive !== 0,
            debugenabled: data.HttpDebug !== 0,
            auth: data.HttpAuth,
            authBasicUser: data.HttpAuthBasicLogin,
            authBasicPassword: data.HttpAuthBasicPassword
          };
        }
      }
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem retrieving Http link settings!'), 2500, true);
    });
  }

  private ShowLinks() {
    const _this = this;

    const oTable = $(this.linkhttptableRef.nativeElement).dataTable({
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
            $('#linkhttpparamstable #linkupdate').attr('class', 'btnstyle3');
            $('#linkhttpparamstable #linkdelete').attr('class', 'btnstyle3');
            const anSelected = DatatableHelper.fnGetSelected(oTable);
            if (anSelected.length !== 0) {
              const data = oTable.fnGetData(anSelected[0]) as DataRow;

              _this.linkToEdit = {
                idx: data.DT_RowId,
                targettype: data.TargetType,
                sendvaluescene: data.TargetType === '2' ? data.Delimitedvalue : undefined,
                onoffdevicename: data.TargetType === '2' || data.TargetType === '3' ? data.DeviceID : undefined,
                sendvaluereboot: data.TargetType === '3' ? data.Delimitedvalue : undefined,
                devicename: data.TargetType === '3' ? undefined : data.DeviceID,
                sendvalue: data.TargetType === '3' ? undefined : data.Delimitedvalue,
                targetvariable: data['3'],
                targetdeviceid: data['4'],
                targetproperty: data['5'],
                linkenabled: data.Enabled === '1',
                includeunit: data.IncludeUnit === '1'
              };

              if (data.TargetType !== '3') {
                _this.ValueSelectorUpdate();
              }
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
    $('#linkhttpparamstable #linkupdate').attr('class', 'btnstyle3-dis');
    $('#linkhttpparamstable #linkdelete').attr('class', 'btnstyle3-dis');

    const oTable = $(this.linkhttptableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.httpService.getHttpLinks().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          let enabled = this.translationService.t('No');
          let includeUnit = this.translationService.t('No');
          if (item.Enabled === '1') {
            enabled = this.translationService.t('Yes');
          }
          if (item.IncludeUnit === '1') {
            includeUnit = this.translationService.t('Yes');
          }
          let TargetType = this.translationService.t('Global variable');
          if (item.TargetType === '1') {
            TargetType = this.translationService.t('Virtual device');
          }
          if (item.TargetType === '2') {
            TargetType = this.translationService.t('Scene');
            includeUnit = '-';
          }
          if (item.TargetType === '3') {
            TargetType = this.translationService.t('Reboot');
            includeUnit = '-';
          }
          if (item.TargetDevice === '0') {
            item.TargetDevice = '';
          }

          let DelimitedValueObs = of('');
          if (item.TargetType === '2') {
            if (item.Delimitedvalue === '0') {
              DelimitedValueObs = of(this.translationService.t('Off'));
            } else if (item.Delimitedvalue === '1') {
              DelimitedValueObs = of(this.translationService.t('On'));
            }
          } else if (item.TargetType === '3') {
            if (item.Delimitedvalue === '0') {
              DelimitedValueObs = of(this.translationService.t('Off'));
            } else if (item.Delimitedvalue === '1') {
              DelimitedValueObs = of(this.translationService.t('On'));
            }
          } else {
            if (item.Delimitedvalue === '0') {
              DelimitedValueObs = of(this.translationService.t('Status'));
            } else {
              DelimitedValueObs = this.GetDeviceValueOptionWording(item.DeviceID, item.Delimitedvalue).pipe(
                map(wording => {
                  return this.translationService.t(wording);
                })
              );
            }
          }

          DelimitedValueObs.subscribe(DelimitedValue => {
            oTable.fnAddData(<DataRow>{
              'DT_RowId': item.idx,
              'TargetType': item.TargetType,
              'DeviceID': item.DeviceID,
              'Enabled': item.Enabled,
              'IncludeUnit': item.IncludeUnit,
              'Delimitedvalue': item.Delimitedvalue,
              '0': item.Name,
              '1': DelimitedValue,
              '2': TargetType,
              '3': item.TargetVariable,
              '4': item.TargetDevice,
              '5': item.TargetProperty,
              '6': includeUnit,
              '7': enabled
            });
          });
        });
      }
    });
  }

  GetDeviceValueOptionWording(idx: string, pos: string): Observable<string> {
    return this.deviceService.getDeviceValueOptionWording(idx, pos).pipe(
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
      this.sendvalues = data.result;
    });
  }

  AddLink(type: string) {
    let idx = this.linkToEdit.idx;
    if (type === 'a') {
      idx = '0';
    }
    let deviceid = this.linkToEdit.devicename;
    let valuetosend = this.linkToEdit.sendvalue;
    let linkactive = 0;
    if (this.linkToEdit.linkenabled) {
      linkactive = 1;
    }
    let includeunit = 0;
    if (this.linkToEdit.includeunit) {
      includeunit = 1;
    }

    let otherparams = {};
    if (this.linkToEdit.targettype === '0') {
      otherparams = {
        targetvariable: this.linkToEdit.targetvariable,
        includeunit: includeunit
      };
    } else if (this.linkToEdit.targettype === '1') {
      otherparams = {
        targetdeviceid: this.linkToEdit.targetdeviceid,
        targetproperty: this.linkToEdit.targetproperty,
        includeunit: includeunit
      };
    } else if (this.linkToEdit.targettype === '2') {
      deviceid = this.linkToEdit.onoffdevicename;
      valuetosend = this.linkToEdit.sendvaluescene;
      otherparams = {
        targetdeviceid: this.linkToEdit.targetdeviceid
      };
    } else if (this.linkToEdit.targettype === '3') {
      deviceid = this.linkToEdit.onoffdevicename;
      valuetosend = this.linkToEdit.sendvaluereboot;
      otherparams = {
        targetdeviceid: '0'
      };
    }

    this.httpService.saveHttpLink(idx, deviceid, valuetosend, this.linkToEdit.targettype, linkactive, otherparams).subscribe(data => {
      bootbox.alert(this.translationService.t('Http link saved'));
      this.RefreshLinkTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving Http link!'), 2500, true);
    });
  }

  DeleteLink(idx: string) {
    bootbox.confirm(this.translationService.t('Are you sure you want to remove this link?'), (result) => {
      if (result === true) {
        this.httpService.deleteLink(idx).subscribe(data => {
          // bootbox.alert(this.translationService.t('Link deleted!'));
          this.RefreshLinkTable();
        });
      }
    });
  }

  SaveConfiguration() {
    let linkactive = 0;
    if (this.httpConfig.linkenabled) {
      linkactive = 1;
    }
    let debugenabled = 0;
    if (this.httpConfig.debugenabled) {
      debugenabled = 1;
    }

    this.httpService.saveHttpLinkConfiguration(this.httpConfig.url, this.httpConfig.method, this.httpConfig.headers, this.httpConfig.data, linkactive, debugenabled,
      this.httpConfig.auth, this.httpConfig.authBasicUser, this.httpConfig.authBasicPassword).subscribe(() => {
      bootbox.alert(this.translationService.t('Http link settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving Http link settings!'), 2500, true);
    });
  }

}

interface HttpConfigUI {
  url: string;
  method: number;
  data: string;
  headers: string;
  linkenabled: boolean;
  debugenabled: boolean;
  auth: number;
  authBasicUser: string;
  authBasicPassword: string;
}

interface DataRow {
  'DT_RowId': string;
  'TargetType': string;
  'DeviceID': string;
  'Enabled': string;
  'IncludeUnit': string;
  'Delimitedvalue': string;
  '0': string;
  '1': string;
  '2': string;
  '3': string;
  '4': string;
  '5': string;
  '6': string;
  '7': string;
}

interface LinkToEdit {
  idx: string;
  targettype: string;
  sendvaluescene: string;
  onoffdevicename: string;
  sendvaluereboot: string;
  devicename: string;
  sendvalue: string;
  targetvariable: string;
  targetdeviceid: string;
  targetproperty: string;
  linkenabled: boolean;
  includeunit: boolean;
}
