import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {DpGooglePubSubService} from './dp-google-pub-sub.service';
import {DeviceService} from '../../../_shared/_services/device.service';
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
  selector: 'dz-dp-google-pub-sub',
  templateUrl: './dp-google-pub-sub.component.html',
  styleUrls: ['./dp-google-pub-sub.component.css']
})
export class DpGooglePubSubComponent implements OnInit, AfterViewInit {

  @ViewChild('linkgooglepubsubtable', {static: false}) linkgooglepubsubtableRef: ElementRef;

  linkToEdit: LinkToEditUI = {
    idx: '0',
    devicename: '',
    onoffdevicename: '',
    sendvalue: '',
    sendvaluereboot: '',
    sendvaluescene: '',
    targetdeviceid: '',
    targetproperty: '',
    targettype: '',
    targetvariable: '',
    linkenabled: true,
    includeunit: true
  };

  config: ConfigUI = {
    data: '',
    debugenabled: true,
    linkenabled: true
  };

  devices: Array<{ name: string; value: string }> = [];
  onOffDevices: Array<{ name: string; value: string }> = [];
  sendvalues: Array<DeviceValueOption>;

  constructor(private googlePubSubService: DpGooglePubSubService,
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

    this.deviceService.listOnOffDevices().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.onOffDevices = data.result;
      }
    });

    this.GetConfig();
  }

  GetConfig() {
    this.googlePubSubService.getConfig().subscribe(data => {
      if (typeof data !== 'undefined') {
        if (data.status === 'OK') {
          this.config = {
            data: data.GooglePubSubData,
            linkenabled: data.GooglePubSubActive !== 0,
            debugenabled: data.GooglePubSubDebug !== 0
          };
        }
      }
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem retrieving Google PubSub link settings!'), 2500, true);
    });
  }

  ngAfterViewInit(): void {
    this.ShowLinks();
  }

  private ShowLinks() {
    const _this = this;

    const oTable = $(this.linkgooglepubsubtableRef.nativeElement).dataTable({
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
            $('#linkgooglepubsubparamstable #linkupdate').attr('class', 'btnstyle3');
            $('#linkgooglepubsubparamstable #linkdelete').attr('class', 'btnstyle3');
            const anSelected = DatatableHelper.fnGetSelected(oTable);
            if (anSelected.length !== 0) {
              const data = oTable.fnGetData(anSelected[0]) as DataRow;
              _this.linkToEdit = {
                idx: data.DT_RowId,
                targettype: data.TargetType,
                sendvaluescene: data.TargetType === '2' ? data.Delimitedvalue : undefined,
                onoffdevicename: data.TargetType === '2' || data.TargetType === '3' ? data.DeviceID : undefined,
                sendvaluereboot: data.TargetType === '3' ? data.Delimitedvalue : undefined,
                devicename: data.TargetType !== '3' ? data.DeviceID : undefined,
                sendvalue: data.TargetType !== '3' ? data.Delimitedvalue : undefined,
                targetvariable: data['3'],
                targetdeviceid: data['4'],
                targetproperty: data['5'],
                linkenabled: data.Enabled === '1',
                includeunit: data.IncludeUnit === '1'
              };
              if (data['TargetType'] !== '3') {
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
    $('#linkgooglepubsubparamstable #linkupdate').attr('class', 'btnstyle3-dis');
    $('#linkgooglepubsubparamstable #linkdelete').attr('class', 'btnstyle3-dis');

    const oTable = $(this.linkgooglepubsubtableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.googlePubSubService.getLinks().subscribe(data => {
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

  GetDeviceValueOptionWording(deviceid: string, pos: string): Observable<string> {
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

    let otherparams = {};
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
        targetdeviceid: '0'
      };
    }

    this.googlePubSubService.saveLink(idx, deviceid, valuetosend, targettype, linkactive, otherparams).subscribe(data => {
      bootbox.alert(this.translationService.t('Google PubSub link saved'));
      this.RefreshLinkTable();
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving Google PubSub link!'), 2500, true);
    });
  }

  DeleteLink(idx: string) {
    bootbox.confirm(this.translationService.t('Are you sure you want to remove this link?'), (result) => {
      if (result === true) {
        this.googlePubSubService.deleteLink(idx).subscribe(data => {
          // bootbox.alert(this.translationService.t('Link deleted!'));
          this.RefreshLinkTable();
        });
      }
    });
  }

  SaveConfiguration() {
    let linkactive = 0;
    if (this.config.linkenabled) {
      linkactive = 1;
    }
    let debugenabled = 0;
    if (this.config.debugenabled) {
      debugenabled = 1;
    }

    this.googlePubSubService.saveConfig(this.config.data, linkactive, debugenabled).subscribe(data => {
      bootbox.alert(this.translationService.t('Google PubSub link settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem saving Google PubSub link settings!'), 2500, true);
    });
  }

}

interface LinkToEditUI {
  idx: string;
  targettype: string;
  devicename: string;
  onoffdevicename: string;
  sendvalue: string;
  sendvaluescene: string;
  sendvaluereboot: string;
  targetvariable: string;
  targetdeviceid: string;
  targetproperty: string;
  includeunit: boolean;
  linkenabled: boolean;
}

interface ConfigUI {
  data: string;
  linkenabled: boolean;
  debugenabled: boolean;
}

interface DataRow {
  DT_RowId: string;
  TargetType: string;
  DeviceID: string;
  Enabled: string;
  IncludeUnit: string;
  Delimitedvalue: string;
  '0': string;
  '1': string;
  '2': string;
  '3': string;
  '4': string;
  '5': string;
  '6': string;
  '7': string;
}
