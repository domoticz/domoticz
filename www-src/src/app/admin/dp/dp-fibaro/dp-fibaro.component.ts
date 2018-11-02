import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {DeviceService} from '../../../_shared/_services/device.service';
import {FibaroService} from './fibaro.service';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DatatableHelper} from '../../../_shared/_utils/datatable-helper';
import {map} from 'rxjs/operators';
import {Observable, of} from 'rxjs';
import {DeviceValueOption} from "../../../_shared/_models/device";
import {ConfigService} from "../../../_shared/_services/config.service";

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-dp-fibaro',
  templateUrl: './dp-fibaro.component.html',
  styleUrls: ['./dp-fibaro.component.css']
})
export class DpFibaroComponent implements OnInit, AfterViewInit {

  @ViewChild('linktable', {static: false}) linktableRef: ElementRef;

  devices: Array<{ name: string; value: string; }> = [];
  onOffDevices: Array<{ name: string; value: string }> = [];

  fibaroConfig: FibaroConfigUI = {
    tcpaddress: '',
    username: '',
    password: '',
    linkenabled: false,
    version4: false,
    debugenabled: false
  };

  editMode = false;
  linkToEdit: LinkToEdit = {
    idx: '0',
    targettype: '0',
    sendvaluescene: '0',
    sendvaluereboot: '0',
    onoffdevicename: '',
    devicename: '',
    sendvalue: '',
    targetvariable: '',
    targetdeviceid: '',
    targetproperty: '',
    linkenabled: false,
    includeunit: false
  };
  sendValueOptions: Array<DeviceValueOption> = [];

  constructor(private deviceService: DeviceService,
              private fibaroService: FibaroService,
              private notificationService: NotificationService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private configService: ConfigService) {
  }

  ngOnInit() {
    // Get devices
    this.deviceService.listDevices().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.devices = data.result;
      }
      this.ValueSelectorUpdate();
    });

    // Get devices On/Off
    this.deviceService.listOnOffDevices().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.onOffDevices = data.result;
      }
    });

    this.GetConfig();
  }

  private GetConfig() {
    this.fibaroService.getConfig().subscribe(data => {
      if (typeof data !== 'undefined') {
        if (data.status === 'OK') {
          this.fibaroConfig = {
            tcpaddress: data.FibaroIP,
            username: data.FibaroUsername,
            password: data.FibaroPassword,
            linkenabled: data.FibaroActive !== 0,
            version4: data.FibaroVersion4 !== 0,
            debugenabled: data.FibaroDebug !== 0
          };
        }
      }
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem retrieving Fibaro link settings!'), 2500, true);
    });
  }

  ngAfterViewInit(): void {
    this.ShowLinks();
  }

  private ShowLinks() {
    const _this = this;

    const oTable = $(this.linktableRef.nativeElement).dataTable({
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

              _this.editMode = true;
              _this.linkToEdit = {
                idx: data['DT_RowId'],
                targettype: data['TargetType'],
                sendvaluescene: data.TargetType === '2' ? data['Delimitedvalue'] : undefined,
                sendvaluereboot: data.TargetType === '3' ? data['Delimitedvalue'] : undefined,
                onoffdevicename: data.TargetType === '2' || data.TargetType === '3' ? data['DeviceID'] : undefined,
                devicename: data.TargetType === '3' ? undefined : data['DeviceID'],
                sendvalue: data.TargetType === '3' ? undefined : data['Delimitedvalue'],
                targetvariable: data['3'],
                targetdeviceid: data['4'],
                targetproperty: data['5'],
                linkenabled: data['Enabled'] === '1',
                includeunit: data['IncludeUnit'] === '1'
              };
              if (_this.linkToEdit.targettype !== '3') {
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

    _this.RefreshLinkTable();
  }

  private RefreshLinkTable() {
    $('#linkparamstable #linkupdate').attr('class', 'btnstyle3-dis');
    $('#linkparamstable #linkdelete').attr('class', 'btnstyle3-dis');

    const oTable = $(this.linktableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.fibaroService.getLinks().subscribe(data => {
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

  private GetDeviceValueOptionWording(idx: string, pos: string): Observable<string> {
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
    this.sendValueOptions = [];
    this.deviceService.getDeviceValueOptions(deviceid).subscribe(data => {
      this.sendValueOptions = data.result;
    });
  }

  AddLink(type: string) {
    let idx = this.linkToEdit.idx;
    if (type === 'a') {
      idx = '0';
    }
    let deviceid = this.linkToEdit.devicename;
    let valuetosend = this.linkToEdit.sendvalue;
    const targettype = this.linkToEdit.targettype;

    let linkactive = 0;
    if (this.linkToEdit.linkenabled) {
      linkactive = 1;
    }

    let includeunit = 0;
    if (this.linkToEdit.includeunit) {
      includeunit = 1;
    }

    let otherparams: any = {};
    if (targettype === '0') {
      otherparams = {
        targetvariable: this.linkToEdit.targetvariable,
        includeunit: includeunit
      };
    } else if (targettype === '1') {
      otherparams = {
        targetdeviceid: this.linkToEdit.targetdeviceid,
        targetproperty: this.linkToEdit.targetproperty,
        includeunit: includeunit
      };
    } else if (targettype === '2') {
      deviceid = this.linkToEdit.onoffdevicename;
      valuetosend = this.linkToEdit.sendvaluescene;
      otherparams = {
        targetdeviceid: this.linkToEdit.targetdeviceid
      };
    } else if (targettype === '3') {
      deviceid = this.linkToEdit.onoffdevicename;
      valuetosend = this.linkToEdit.sendvaluereboot;
      otherparams = {
        targetdeviceid: '0',
      };
    }

    this.fibaroService.saveFibaroLink(idx, deviceid, valuetosend, targettype, linkactive, otherparams).subscribe(data => {
      bootbox.alert(this.translationService.t('Fibaro link saved'));
      this.RefreshLinkTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving Fibaro link!'), 2500, true);
    });
  }

  DeleteLink(idx: string) {
    bootbox.confirm(this.translationService.t('Are you sure you want to remove this link?'), (result) => {
      if (result === true) {
        this.fibaroService.deleteFibaroLink(idx).subscribe(() => {
          // bootbox.alert(this.translationService.t('Link deleted!'));
          this.RefreshLinkTable();
        });
      }
    });
  }

  SaveConfiguration() {
    const remoteurl = this.fibaroConfig.tcpaddress;
    let cleanurl = remoteurl;
    if (remoteurl.indexOf('://') > 0) {
      cleanurl = remoteurl.substr(remoteurl.indexOf('://') + 3);
    }
    const username = this.fibaroConfig.username;
    const password = this.fibaroConfig.password;
    let linkactive = 0;
    if (this.fibaroConfig.linkenabled) {
      linkactive = 1;
    }
    let isversion4 = 0;
    if (this.fibaroConfig.version4) {
      isversion4 = 1;
    }
    let debugenabled = 0;
    if (this.fibaroConfig.debugenabled) {
      debugenabled = 1;
    }

    this.fibaroService.saveFibaroLinkConfig(cleanurl, username, password, linkactive, isversion4, debugenabled).subscribe(() => {
      bootbox.alert(this.translationService.t('Fibaro link settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving Fibaro link settings!'), 2500, true);
    });
  }

}

interface FibaroConfigUI {
  tcpaddress: string;
  username: string;
  password: string;
  linkenabled: boolean;
  version4: boolean;
  debugenabled: boolean;
}

interface LinkToEdit {
  idx: string;
  targettype: string;
  sendvaluescene: string;
  sendvaluereboot: string;
  onoffdevicename: string;
  devicename: string;
  sendvalue: string;
  targetvariable: string;
  targetdeviceid: string;
  targetproperty: string;
  linkenabled: boolean;
  includeunit: boolean;
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
