import {EvoHomeResponse, HardwareTypeParameterOption, RegisterHueResponse} from '../../../_shared/_models/hardware';
import {HardwareService} from '../../../_shared/_services/hardware.service';
import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {ConfigService} from 'src/app/_shared/_services/config.service';
import {Hardware, HardwareType, HardwareTypeParameter} from 'src/app/_shared/_models/hardware';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DatatableHelper} from 'src/app/_shared/_utils/datatable-helper';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {timer} from 'rxjs';
import {Router} from '@angular/router';
import {DialogService} from 'src/app/_shared/_services/dialog.service';
import {CreateDummySensorDialogComponent} from '../../../_shared/_dialogs/create-dummy-sensor-dialog/create-dummy-sensor-dialog.component';
import {AddYeeLightDialogComponent} from '../../../_shared/_dialogs/add-yee-light-dialog/add-yee-light-dialog.component';
import {CreateRfLinkDeviceDialogComponent} from '../../../_shared/_dialogs/create-rf-link-device-dialog/create-rf-link-device-dialog.component';
import {CreateEvohomeSensorsDialogComponent} from '../../../_shared/_dialogs/create-evohome-sensors-dialog/create-evohome-sensors-dialog.component';
import {delay, mergeMap, tap} from 'rxjs/operators';
import {AddAriluxDialogComponent} from '../../../_shared/_dialogs/add-arilux-dialog/add-arilux-dialog.component';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-hardware',
  templateUrl: './hardware.component.html',
  styleUrls: ['./hardware.component.css']
})
export class HardwareComponent implements OnInit, AfterViewInit {

  devIdx = 0;
  SerialPortStr: string[] = [];
  serialDevices: { name: string; value: number; }[] = [];
  hardwareTypes: HardwareType[] = [];
  hardwareI2CTypes: { idx: number; name: string }[] = [];

  @ViewChild('hardwaretable', {static: false}) hardwareTableElt: ElementRef;
  hardwareTable: any;

  selectedRow: HardwareRow;
  selectedHardware: SelectedHardware = {};
  devExtra: any;

  enabledDisabled = false;
  hardwarenameDisabled = false;
  typeDisabled = false;
  timeoutDisabled = false;
  usernameDisplay = false;
  divehouseDisplay = false;
  divevohomeDisplay = false;
  divevohometcpDisplay = false;
  divevohomewebDisplay = false;
  divusbtinDisplay = false;
  divbaudratemysensorsDisplay = false;
  divbaudratep1Display = false;
  divbaudrateteleinfoDisplay = false;
  divmodelecodevicesDisplay = false;
  divcrcp1Display = false;
  divratelimitp1Display = false;
  divlocationDisplay = false;
  divphilipshueDisplay = false;
  divwinddelenDisplay = false;
  divhoneywellDisplay = false;
  divmqttDisplay = false;
  divmysensorsmqttDisplay = false;
  divsolaredgeapiDisplay = false;
  divnestoauthapiDisplay = false;
  divenecotoonDisplay = false;
  div1wireDisplay = false;
  divgoodwewebDisplay = false;
  divi2clocalDisplay = false;
  divi2caddressDisplay = false;
  divi2cinvertDisplay = false;
  divpollintervalDisplay = false;
  divpythonpluginDisplay = false;
  divrelaynetDisplay = false;
  divgpioDisplay = false;
  divsysfsgpioDisplay = false;
  divmodeldenkovidevicesDisplay = false;
  divmodeldenkoviusbdevicesDisplay = false;
  divmodeldenkovitcpdevicesDisplay = false;
  divserialDisplay = false;
  divremoteDisplay = false;
  divloginDisplay = false;
  divundergroundDisplay = false;
  divbuienradarDisplay = false;
  divhttppollerDisplay = false;
  divteslaDisplay = false;
  passwordDisplay = false;
  denkovislaveidDisplay = false;
  divrtl433Display = false;
  mqtt_publishDisplay = false;
  postdataDisplay = false;

  range10: number[];

  millselectOptions = [
    {value: 1, label: 'De Grote Geert'},
    {value: 2, label: 'De Jonge Held'},
    {value: 31, label: 'Het Rode Hert'},
    {value: 41, label: 'De Ranke Zwaan'},
    {value: 51, label: 'De Witte Juffer'},
    {value: 111, label: 'De Bonte Hen'},
    {value: 121, label: 'De Trouwe Wachter'},
    {value: 131, label: 'De Blauwe Reiger'},
    {value: 141, label: 'De Vier Winden'},
    {value: 191, label: 'De Boerenzwaluw'}
  ];

  hardwareupdateenabled = false;
  hardwaredeleteenabled = false;

  constructor(
    public configService: ConfigService,
    private hardwareService: HardwareService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private notificationService: NotificationService,
    private dialogService: DialogService,
    private router: Router
  ) {
  }

  ngOnInit() {
    this.hardwareService.getSerialDevices().subscribe(data => {
      this.serialDevices = data.result;
      this.SerialPortStr = this.serialDevices.map(_ => _.name);
    });

    // Get hardware types
    this.hardwareService.getHardwareTypes().subscribe(data => {
      data.result.sort(this.SortByName);  // Plugins will not be in order so sort the array
      this.hardwareTypes = [];
      this.hardwareI2CTypes = [];

      data.result.forEach((item: HardwareType) => {
        this.configService.globals.HardwareTypesStr[item.idx] = item.name;
        // Don't show I2C sensors
        if (item.name.indexOf('I2C sensor') !== -1) {
          this.configService.globals.HardwareI2CStr[item.idx] = item.name;
          this.hardwareI2CTypes.push({idx: item.idx, name: item.name});
        } else {
          this.hardwareTypes.push(item);
        }
      });
    });
  }

  ngAfterViewInit() {
    this.ShowHardware();
  }

  public isParamSerialPort(param: HardwareTypeParameter): boolean {
    return typeof (param.options) === 'undefined' && param.field === 'SerialPort';
  }

  public isNotParamSerialPort(param: HardwareTypeParameter): boolean {
    return typeof (param.options) === 'undefined' && param.field !== 'SerialPort';
  }

  public isParamPassword(param: HardwareTypeParameter): boolean {
    return (typeof (param.password) !== 'undefined') && (param.password === 'true');
  }

  public isParamDefault(param: HardwareTypeParameter): boolean {
    return typeof (param.default) !== 'undefined';
  }

  public isParamRequired(param: HardwareTypeParameter): boolean {
    return (typeof (param.required) !== 'undefined') && (param.required === 'true');
  }

  public isParamOptions(param: HardwareTypeParameter): boolean {
    return typeof (param.options) !== 'undefined';
  }

  public isOptionDefault(option: HardwareTypeParameterOption) {
    return (typeof (option.default) !== 'undefined') && (option.default === 'true');
  }

  private SortByName(a: HardwareType, b: HardwareType): number {
    const aName = a.name.toLowerCase();
    const bName = b.name.toLowerCase();
    return ((aName < bName) ? -1 : ((aName > bName) ? 1 : 0));
  }

  private ShowHardware() {

    this.hardwareTable = $(this.hardwareTableElt.nativeElement).dataTable({
      sDom: '<"H"lfrC>t<"F"ip>',
      oTableTools: {
        sRowSelect: 'single',
      },
      aaSorting: [[0, 'desc']],
      bSortClasses: false,
      bProcessing: true,
      bStateSave: true,
      bJQueryUI: true,
      aLengthMenu: [[25, 50, 100, -1], [25, 50, 100, 'All']],
      iDisplayLength: 25,
      sPaginationType: 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });

    this.RefreshHardwareTable();
    // this.UpdateHardwareParamControls();

  }

  private RefreshHardwareTable() {
    this.hardwareTable.fnClearTable();

    this.hardwareupdateenabled = false;
    this.hardwaredeleteenabled = false;

    const _this = this;

    this.hardwareService.getHardwares().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item: Hardware) => {

          let HwTypeStrOrg = this.configService.globals.HardwareTypesStr[item.Type];
          let HwTypeStr = HwTypeStrOrg;
          const hardwareSetupLink = '<a href="/Hardware/' + item.idx + '" class="label label-info lcursor btn-link ng-link">' +
            this.translationService.t('Setup') + '</a>';

          if (typeof HwTypeStr === 'undefined') {
            HwTypeStr = '???? Unknown (NA/Not supported)';
          }

          let SerialName = 'Unknown!?';
          let intport = 0;
          if ((HwTypeStr.indexOf('LAN') >= 0) || (HwTypeStr.indexOf('MySensors Gateway with MQTT') >= 0) ||
            (HwTypeStr.indexOf('Domoticz') >= 0) || (HwTypeStr.indexOf('Harmony') >= 0) || (HwTypeStr.indexOf('Philips Hue') >= 0)) {
            SerialName = item.Port.toString();
          } else if ((item.Type === 7) || (item.Type === 11)) {
            SerialName = 'USB';
          } else if ((item.Type === 14) || (item.Type === 25) || (item.Type === 28) || (item.Type === 30) || (item.Type === 34)) {
            SerialName = 'WWW';
          } else if ((item.Type === 15) || (item.Type === 23) || (item.Type === 26) || (item.Type === 27) || (item.Type === 51) ||
            (item.Type === 54)) {
            SerialName = '';
          } else if ((item.Type === 16) || (item.Type === 32)) {
            SerialName = 'GPIO';
          } else if (HwTypeStr.indexOf('Evohome') >= 0 && HwTypeStr.indexOf('script') >= 0) {
            SerialName = 'Script';
          } else if (item.Type === 74) {
            intport = item.Port;
            SerialName = '';
          } else if (item.Type === 94) {
            // FIXME use Angular
            HwTypeStr = $('#' + item.Extra).text();
            HwTypeStrOrg = 'PLUGIN';
            intport = item.Port;
            SerialName = item.SerialPort;
          } else {
            SerialName = item.SerialPort;
            intport = this.SerialPortStr.findIndex(_ => _ === item.SerialPort);
          }

          let enabledstr = this.translationService.t('No');
          if (item.Enabled === 'true') {
            enabledstr = this.translationService.t('Yes');
          }

          if (HwTypeStr.indexOf('RFXCOM') >= 0) {
            HwTypeStr += '<br>Version: ' + item.version;
            if (item.noiselvl !== 0) {
              HwTypeStr += ', Noise: ' + item.noiselvl + ' dB';
            }
            if (HwTypeStr.indexOf('868') >= 0) {
              HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditRFXCOMMode868/' + item.idx + '">' +
                this.translationService.t('Set Mode') + '</a>';
            } else {
              HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditRFXCOMMode/' + item.idx + '">' +
                this.translationService.t('Set Mode') + '</a>';
            }
          } else if (HwTypeStr.indexOf('S0 Meter') >= 0) {
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditS0MeterType/' + item.idx + '">' +
              this.translationService.t('Set Mode') + '</a>';
          } else if (HwTypeStr.indexOf('Limitless') >= 0) {
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditLimitlessType/' + item.idx + '">' +
              this.translationService.t('Set Mode') + '</a>';
          } else if (HwTypeStr.indexOf('OpenZWave') >= 0) {
            HwTypeStr += '<br>Version: ' + item.version;

            if (typeof item.NodesQueried !== 'undefined') {
              let lblStatus = 'label-info';
              if (item.NodesQueried !== true) {
                lblStatus = 'label-important';
              }
              HwTypeStr += ' <a href="/Hardware/' + item.idx + '" class="label ' + lblStatus + ' btn-link ng-link">' +
                this.translationService.t('Setup') + '</a>';
            }
          } else if (HwTypeStr.indexOf('SBFSpot') >= 0) {
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditSBFSpot/' + item.idx + '">' +
              this.translationService.t('Setup') + '</a>';
          } else if (HwTypeStr.indexOf('MySensors') >= 0) {
            HwTypeStr += '<br>Version: ' + item.version;
            HwTypeStr += ' ' + hardwareSetupLink;
          } else if ((HwTypeStr.indexOf('OpenTherm') >= 0) || (HwTypeStr.indexOf('Thermosmart') >= 0)) {
            HwTypeStr += '<br>Version: ' + item.version;
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditOpenTherm/' + item.idx + '">' +
              this.translationService.t('Setup') + '</a>';
          } else if (HwTypeStr.indexOf('Wake-on-LAN') >= 0) {
            HwTypeStr += ' ' + hardwareSetupLink;
          } else if (HwTypeStr.indexOf('System Alive') >= 0) {
            HwTypeStr += ' ' + hardwareSetupLink;
          } else if (HwTypeStr.indexOf('Kodi Media') >= 0) {
            HwTypeStr += ' ' + hardwareSetupLink;
          } else if (HwTypeStr.indexOf('Panasonic') >= 0) {
            HwTypeStr += ' ' + hardwareSetupLink;
          } else if (HwTypeStr.indexOf('BleBox') >= 0) {
            HwTypeStr += ' ' + hardwareSetupLink;
          } else if (HwTypeStr.indexOf('Logitech Media Server') >= 0) {
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditLMS/' + item.idx + '">' +
              this.translationService.t('Setup') + '</a>';
          } else if (HwTypeStr.indexOf('HEOS by DENON') >= 0) {
            // FIXME WTF function with space which obviously does not exist...
            // HwTypeStr += ' <span class="label label-info lcursor" onclick="EditHEOS by DENON(' + item.idx + ',\'' + item.Name + '\',' +
            //   item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' +
            //   this.translationService.t('Setup') + '</span>';
          } else if (HwTypeStr.indexOf('Dummy') >= 0) {
            HwTypeStr += ' <span class="label label-info lcursor dummy-dialog" dummydialogidx="' + item.idx +
              '" dummydialogname="' + item.Name + '">' +
              this.translationService.t('Create Virtual Sensors') + '</span>';
          } else if (HwTypeStr.indexOf('YeeLight') >= 0) {
            HwTypeStr += ' <span class="label label-info lcursor yee-dialog yeedialogidx="' + item.idx +
              '" yeedialogname="' + item.Name + '">' +
              this.translationService.t('Add Light') + '</span>';
          } else if (HwTypeStr.indexOf('PiFace') >= 0) {
            HwTypeStr += ' <span class="label label-info lcursor reloadpiface" reloadpifaceidx="' + item.idx + '">' +
              this.translationService.t('Reload') + '</span>';
          } else if (HwTypeStr.indexOf('HTTP/HTTPS') >= 0) {
            HwTypeStr += ' <span class="label label-info lcursor dummy-dialog" dummydialogidx="' + item.idx +
              '" dummydialogname="' + item.Name + '">' +
              this.translationService.t('Create Virtual Sensors') + '</span>';
          } else if (HwTypeStr.indexOf('RFLink') >= 0) {
            HwTypeStr += '<br>Version: ' + item.version;
            HwTypeStr += ' <span class="label label-info lcursor rflink-dialog" rflinkdialogidx="' + item.idx + '">' +
              this.translationService.t('Create RFLink Devices') + '</span>';
          } else if (HwTypeStr.indexOf('Evohome') >= 0) {
            if (HwTypeStr.indexOf('script') >= 0 || HwTypeStr.indexOf('Web') >= 0) {
              HwTypeStr += ' <span class="label label-info lcursor evohome-dialog" evohomedialogidx="' + item.idx + '">' +
                this.translationService.t('Create Devices') + '</span>';
            } else {
              HwTypeStr += ' <span class="label label-info lcursor bindevohome" bindevohomeidx="' + item.idx + '"' +
                ' bindevohometype="Relay">Bind Relay</span>';
              HwTypeStr += ' <span class="label label-info lcursor bindevohome" bindevohomeidx="' + item.idx + '"' +
                ' bindevohometype="OutdoorSensor">Outdoor Sensor</span>';
              HwTypeStr += ' <span class="label label-info lcursor bindevohome" bindevohomeidx="' + item.idx + '"' +
                ' bindevohometype="AllSensors">All Sensors</span>';
              HwTypeStr += ' <span class="label label-info lcursor bindevohome" bindevohomeidx="' + item.idx + '"' +
                ' bindevohometype="ZoneSensor">Bind Temp Sensor</span>';
            }
          } else if (HwTypeStr.indexOf('Rego 6XX') >= 0) {
            HwTypeStr += '<br>Type: ';
            if (item.Mode1 === 0) {
              HwTypeStr += '600-635, 637 single language';
            } else if (item.Mode1 === 1) {
              HwTypeStr += '636';
            } else if (item.Mode1 === 2) {
              HwTypeStr += '637 multi language';
            }
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditRego6XXType/' +
              item.idx + '">Change Type</a>';
          } else if (HwTypeStr.indexOf('CurrentCost Meter USB') >= 0) {
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/EditCCUSB/' + item.idx + '">' +
              this.translationService.t('Set Mode') + '</a>';
          } else if (HwTypeStr.indexOf('Tellstick') >= 0) {
            HwTypeStr += ' <a class="label label-info lcursor btn-link ng-link" href="/Hardware/TellstickSettings/' + item.idx + '">' +
              this.translationService.t('Settings') + '</a>';
          } else if (HwTypeStr.indexOf('Arilux AL-LC0x') >= 0) {
            HwTypeStr += ' <span class="label label-info lcursor arilux-dialog" ariluxdialogidx="' + item.idx + '">' +
              this.translationService.t('Add Light') + '</span>';
          }


          let sDataTimeout = '';
          if (item.DataTimeout === 0) {
            sDataTimeout = this.translationService.t('Disabled');
          } else if (item.DataTimeout < 60) {
            sDataTimeout = item.DataTimeout + ' ' + this.translationService.t('Seconds');
          } else if (item.DataTimeout < 3600) {
            const minutes = item.DataTimeout / 60;
            if (minutes === 1) {
              sDataTimeout = minutes + ' ' + this.translationService.t('Minute');
            } else {
              sDataTimeout = minutes + ' ' + this.translationService.t('Minutes');
            }
          } else if (item.DataTimeout <= 86400) {
            const hours = item.DataTimeout / 3600;
            if (hours === 1) {
              sDataTimeout = hours + ' ' + this.translationService.t('Hour');
            } else {
              sDataTimeout = hours + ' ' + this.translationService.t('Hours');
            }
          } else {
            const days = item.DataTimeout / 86400;
            if (days === 1) {
              sDataTimeout = days + ' ' + this.translationService.t('Day');
            } else {
              sDataTimeout = days + ' ' + this.translationService.t('Days');
            }
          }

          let dispAddress = item.Address;
          if ((item.Type === 13) || (item.Type === 71) || (item.Type === 85) || (item.Type === 96)) {
            dispAddress = 'I2C';
          } else if (item.Type === 93 || item.Type === 109) {
            dispAddress = 'I2C-' + dispAddress;
          }

          this.hardwareTable.fnAddData({
            DT_RowId: item.idx,
            Username: item.Username,
            Password: item.Password,
            Extra: item.Extra,
            Enabled: item.Enabled,
            Name: item.Name,
            Mode1: item.Mode1,
            Mode2: item.Mode2,
            Mode3: item.Mode3,
            Mode4: item.Mode4,
            Mode5: item.Mode5,
            Mode6: item.Mode6,
            Type: HwTypeStrOrg,
            IntPort: intport,
            Address: item.Address,
            Port: SerialName,
            DataTimeout: item.DataTimeout,
            0: item.idx,
            1: item.Name,
            2: enabledstr,
            3: HwTypeStr,
            4: dispAddress,
            5: SerialName,
            6: sDataTimeout
          });

        });

        // Fix links so that it uses Angular router
        DatatableHelper.fixAngularLinks('.ng-link', this.router);
        $('.dummy-dialog').off().on('click', function (e) {
          e.preventDefault();
          _this.CreateDummySensors(this.dummydialogidx, this.dummydialogname);
        });
        $('.yee-dialog').off().on('click', function (e) {
          e.preventDefault();
          _this.AddYeeLight(this.yeedialogidx, this.yeedialogname);
        });
        $('.reloadpiface').off().on('click', function (e) {
          e.preventDefault();
          _this.ReloadPiFace(this.reloadpifaceidx);
        });
        $('.rflink-dialog').off().on('click', function (e) {
          e.preventDefault();
          _this.CreateRFLinkDevices(this.rflinkdialogidx);
        });
        $('.evohome-dialog').off().on('click', function (e) {
          e.preventDefault();
          _this.CreateEvohomeSensors(this.evohomedialogidx);
        });
        $('.bindevohome').off().on('click', function (e) {
          e.preventDefault();
          _this.BindEvohome(this.bindevohomeidx, this.bindevohometype);
        });
        $('.arilux-dialog').off().on('click', function (e) {
          e.preventDefault();
          _this.AddArilux(this.ariluxdialogidx);
        });
      }

      this.UpdateHardwareParamControls();
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#hardwaretable tbody').off();
    $('#hardwaretable tbody').on('click', 'tr', function () {
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        _this.hardwareupdateenabled = false;
        _this.hardwaredeleteenabled = false;
      } else {
        const oTable = _this.hardwareTable;
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        _this.hardwareupdateenabled = true;
        _this.hardwaredeleteenabled = true;
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data: HardwareRow = oTable.fnGetData(anSelected[0]) as HardwareRow;

          _this.selectedRow = data;

          _this.selectedHardware.name = data.Name;
          if (data.Type !== 'PLUGIN') {
            _this.selectedHardware.type = _this.configService.globals.HardwareTypesStr.findIndex(_ => _ === data.Type);
          } else {
            _this.selectedHardware.type = data.Extra;
          }
          _this.selectedHardware.enabled = (data.Enabled === 'true');
          _this.selectedHardware.timeout = data.DataTimeout;

          if (data.Type.indexOf('I2C ') >= 0) {
            _this.selectedHardware.type = 1000;
          }

          _this.devExtra = data.Extra;
          _this.UpdateHardwareParamControls();

          // Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
          // Handle plugins generically.  If the plugin requires a data field it will have been created on page load.
          if (data.Type === 'PLUGIN') {
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Username').val(data.Username);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Password').val(data.Password);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Address').val(data.Address);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Port').val(data.IntPort);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #SerialPort').val(data.Port);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Mode1').val(data.Mode1);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Mode2').val(data.Mode2);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Mode3').val(data.Mode3);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Mode4').val(data.Mode4);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Mode5').val(data.Mode5);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Mode6').val(data.Mode6);
            $('#hardwarecontent #divpythonplugin #' + data.Extra + ' #Extra').val(data.Extra);
            _this.UpdateHardwareParamControls();
            return;
          }

          if (
            (data.Type.indexOf('TE923') >= 0) ||
            (data.Type.indexOf('Volcraft') >= 0) ||
            (data.Type.indexOf('Dummy') >= 0) ||
            (data.Type.indexOf('System Alive') >= 0) ||
            (data.Type.indexOf('PiFace') >= 0) ||
            (data.Type.indexOf('Tellstick') >= 0) ||
            (data.Type.indexOf('Yeelight') >= 0) ||
            (data.Type.indexOf('Arilux AL-LC0x') >= 0)
          ) {
            // nothing to be set
          } else if (data.Type.indexOf('1-Wire') >= 0) {
            _this.selectedHardware.owfspath = data.Extra;
            _this.selectedHardware.OneWireSensorPollPeriod = data.Mode1;
            _this.selectedHardware.OneWireSwitchPollPeriod = data.Mode2;
          } else if (data.Type.indexOf('I2C ') >= 0) {
            _this.selectedHardware.i2clocal = _this.configService.globals.HardwareI2CStr.findIndex(_ => _ === data.Type);
            _this.selectedHardware.i2cpath = data.Port;
            if (data.Type.indexOf('I2C sensor PIO 8bit expander PCF8574') >= 0) {
              _this.selectedHardware.i2caddress = data.Address;
              _this.selectedHardware.i2cinvert = data.Mode1 === 1;
            } else if (data.Type.indexOf('I2C sensor GPIO 16bit expander MCP23017') >= 0) {
              _this.selectedHardware.i2caddress = data.Address;
              _this.selectedHardware.i2cinvert = data.Mode1 === 1;
            }
          } else if ((data.Type.indexOf('GPIO') >= 0) && (data.Type.indexOf('sysfs GPIO') === -1)) {
            _this.selectedHardware.gpiodebounce = data.Mode1;
            _this.selectedHardware.gpioperiod = data.Mode2;
            _this.selectedHardware.gpiopollinterval = data.Mode3;
          } else if (data.Type.indexOf('sysfs GPIO') >= 0) {
            _this.selectedHardware.sysfsautoconfigure = data.Mode1 === 1;
            _this.selectedHardware.sysfsdebounce = data.Mode2;
          } else if (data.Type.indexOf('USB') >= 0 || data.Type.indexOf('Teleinfo EDF') >= 0) {
            _this.selectedHardware.serialport = data.IntPort;
            if (data.Type.indexOf('Evohome') >= 0) {
              _this.selectedHardware.baudrateevohome = data.Mode1;
              _this.selectedHardware.controllerid = data.Extra;
            }
            if (data.Type.indexOf('MySensors') >= 0) {
              _this.selectedHardware.baudrate = data.Mode1;
            }
            if (data.Type.indexOf('P1 Smart Meter') >= 0) {
              _this.selectedHardware.baudratep1 = data.Mode1;
              _this.selectedHardware.disablecrcp1 = data.Mode2 === 0;
              _this.selectedHardware.ratelimitp1 = data.Mode3;
              if (data.Mode1 === 0) {
                _this.divcrcp1Display = false;
              } else {
                _this.divcrcp1Display = true;
              }
            } else if (data.Type.indexOf('Teleinfo EDF') >= 0) {
              _this.selectedHardware.baudrateteleinfo = data.Mode1;
              _this.selectedHardware.disablecrcp1 = data.Mode2 === 0;
              _this.selectedHardware.ratelimitp1 = data.Mode3;
              if (data.Mode1 === 0) {
                _this.divcrcp1Display = false;
              } else {
                _this.divcrcp1Display = true;
              }
            } else if (data.Type.indexOf('USBtin') >= 0) {
              // $("#hardwarecontent #divusbtin #combotypecanusbtin").val( data["Mode1"] );
              // tslint:disable-next-line:no-bitwise
              _this.selectedHardware.activateMultiblocV8 = (data.Mode1 & 0x01) > 0;
              // tslint:disable-next-line:no-bitwise
              _this.selectedHardware.activateCanFree = (data.Mode1 & 0x02) > 0;
              _this.selectedHardware.debugusbtin = data.Mode2;

            } else if (data.Type.indexOf('Denkovi') >= 0) {
              _this.selectedHardware.modeldenkoviusbdevices = data.Mode1;
            }
          } else if ((((data.Type.indexOf('LAN') >= 0) || (data.Type.indexOf('Eco Devices') >= 0) ||
            data.Type.indexOf('MySensors Gateway with MQTT') >= 0) && (data.Type.indexOf('YouLess') === -1) &&
            (data.Type.indexOf('Denkovi') === -1) && (data.Type.indexOf('Relay-Net') === -1) &&
            (data.Type.indexOf('Satel Integra') === -1) && (data.Type.indexOf('eHouse') === -1) &&
            (data.Type.indexOf('MyHome OpenWebNet with LAN interface') === -1)) || (data.Type.indexOf('Domoticz') >= 0) ||
            (data.Type.indexOf('Harmony') >= 0)) {
            _this.selectedHardware.tcpaddress = data.Address;
            _this.selectedHardware.tcpport = data.Port;
            if (data.Type.indexOf('P1 Smart Meter') >= 0) {
              _this.selectedHardware.disablecrcp1 = data.Mode2 === 0;
              _this.selectedHardware.ratelimitp1 = data.Mode3;
            }
            if (data.Type.indexOf('Eco Devices') >= 0) {
              _this.selectedHardware.modelecodevices = data.Mode1;
              _this.selectedHardware.ratelimitp1 = data.Mode2;
            }
            if (data.Type.indexOf('Evohome') >= 0) {
              _this.selectedHardware.controlleridevohometcp = data.Extra;
            }
          } else if (
            (((data.Type.indexOf('LAN') >= 0) || data.Type.indexOf('MySensors Gateway with MQTT') >= 0) &&
              (data.Type.indexOf('YouLess') >= 0)) ||
            (data.Type.indexOf('Domoticz') >= 0) ||
            (data.Type.indexOf('Denkovi') >= 0) ||
            (data.Type.indexOf('Relay-Net') >= 0) ||
            (data.Type.indexOf('Satel Integra') >= 0) ||
            (data.Type.indexOf('eHouse') >= 0) ||
            (data.Type.indexOf('Logitech Media Server') >= 0) ||
            (data.Type.indexOf('HEOS by DENON') >= 0) ||
            (data.Type.indexOf('Xiaomi Gateway') >= 0) ||
            (data.Type.indexOf('MyHome OpenWebNet with LAN interface') >= 0)
          ) {
            _this.selectedHardware.tcpaddress = data.Address;
            _this.selectedHardware.tcpport = data.Port;
            _this.selectedHardware.password = data.Password;

            if (data.Type.indexOf('Satel Integra') >= 0) {
              _this.selectedHardware.pollinterval = data.Mode1;
            } else if (data.Type.indexOf('MyHome OpenWebNet with LAN interface') >= 0) {
              let RateLimit = data.Mode1;
              if (RateLimit && (RateLimit < 300)) {
                RateLimit = 300;
              }
              _this.selectedHardware.ratelimitp1 = RateLimit;
            } else if (data.Type.indexOf('eHouse') >= 0) {
              _this.selectedHardware.pollinterval = data.Mode1;
              _this.selectedHardware.ehouseautodiscovery = data.Mode2 === 1;
              _this.selectedHardware.ehouseaddalarmin = data.Mode3 === 1;
              _this.selectedHardware.ehouseprodiscovery = data.Mode4 === 1;
              _this.selectedHardware.ehouseopts = data.Mode5;
              _this.selectedHardware.ehouseopts2 = data.Mode6;
            } else if (data.Type.indexOf('Relay-Net') >= 0) {
              _this.selectedHardware.relaynetpollinputs = data.Mode1 === 1;
              _this.selectedHardware.relaynetpollrelays = data.Mode2 === 1;
              _this.selectedHardware.relaynetpollinterval = data.Mode3;
              _this.selectedHardware.relaynetinputcount = data.Mode4;
              _this.selectedHardware.relaynetrelaycount = data.Mode5;
            } else if (data.Type.indexOf('Denkovi') >= 0) {
              _this.selectedHardware.pollinterval = data.Mode1;
              if (data.Type.indexOf('Modules with LAN (HTTP)') >= 0) {
                _this.selectedHardware.modeldenkovidevices = data.Mode2;
              } else if (data.Type.indexOf('Modules with LAN (TCP)') >= 0) {
                _this.selectedHardware.modeldenkovitcpdevices = data.Mode2;
                if (data.Mode2 === 1) {
                  _this.denkovislaveidDisplay = true;
                  _this.selectedHardware.denkovislaveid = data.Mode3;
                } else {
                  _this.denkovislaveidDisplay = false;
                }
              }
            }
          } else if ((data.Type.indexOf('Underground') >= 0) || (data.Type.indexOf('DarkSky') >= 0) ||
            (data.Type.indexOf('AccuWeather') >= 0) || (data.Type.indexOf('Open Weather Map') >= 0)) {
            _this.selectedHardware.apikey = data.Username;
            _this.selectedHardware.location = data.Password;
          } else if ((data.Type.indexOf('Underground') >= 0) || (data.Type.indexOf('DarkSky') >= 0) ||
            (data.Type.indexOf('AccuWeather') >= 0) || (data.Type.indexOf('Open Weather Map') >= 0)) {
            _this.selectedHardware.apikey = data.Username;
            _this.selectedHardware.location = data.Password;
          } else if (data.Type.indexOf('Buienradar') >= 0) {
            let timeframe = Number(data.Mode1);
            let threshold = Number(data.Mode2);
            if (timeframe === 0) {
              timeframe = 15;
            }
            if (threshold === 0) {
              threshold = 25;
            }
            _this.selectedHardware.timeframe = timeframe;
            _this.selectedHardware.threshold = threshold;
            _this.selectedHardware.location = data.Password;
          } else if ((data.Type.indexOf('HTTP/HTTPS') >= 0)) {
            _this.selectedHardware.url = data.Address;
            const tmp = data.Extra;
            const tmparray = tmp.split('|');
            if (tmparray.length >= 4) {
              _this.selectedHardware.script = atob(tmparray[0]);
              _this.selectedHardware.method = Number(atob(tmparray[1]));
              _this.selectedHardware.contenttype = atob(tmparray[2]);
              _this.selectedHardware.headers = atob(tmparray[3]);
              if (tmparray.length >= 5) {
                _this.selectedHardware.postdata = atob(tmparray[4]);
              }
              if (atob(tmparray[1]) === '1') {
                _this.postdataDisplay = true;
              } else {
                _this.postdataDisplay = false;
              }
            }
            _this.selectedHardware.refresh = data.IntPort;
          } else if (data.Type.indexOf('SBFSpot') >= 0) {
            _this.selectedHardware.location = data.Username;
          } else if (data.Type.indexOf('SolarEdge via') >= 0) {
            _this.selectedHardware.apikey = data.Username;
          } else if (data.Type.indexOf('Nest Th') >= 0 && data.Type.indexOf('OAuth') >= 0) {
            _this.selectedHardware.apikey = data.Username;

            const tmp = data.Extra;
            const tmparray = tmp.split('|');
            if (tmparray.length === 3) {
              _this.selectedHardware.productid = atob(tmparray[0]);
              _this.selectedHardware.productsecret = atob(tmparray[1]);
              _this.selectedHardware.productpin = atob(tmparray[2]);
            }
          } else if (data.Type.indexOf('Toon') >= 0) {
            _this.selectedHardware.agreement = data.Mode1;
          } else if (data.Type.indexOf('Tesla') >= 0) {
            _this.selectedHardware.teslavinnr = data.Extra;
            _this.selectedHardware.tesladefaultinterval = Number(data.Mode1);
            _this.selectedHardware.teslaactiveinterval = Number(data.Mode2);
            _this.selectedHardware.teslaallowwakeup = Number(data.Mode3);
          } else if (data.Type.indexOf('Satel Integra') >= 0) {
            _this.selectedHardware.pollinterval = data.Mode1;
          } else if (data.Type.indexOf('eHouse') >= 0) {
            _this.selectedHardware.pollinterval = data.Mode1;
            _this.selectedHardware.ehouseautodiscovery = data.Mode2 === 1;
            _this.selectedHardware.ehouseaddalarmin = data.Mode3 === 1;
            _this.selectedHardware.ehouseprodiscovery = data.Mode4 === 1;
            _this.selectedHardware.ehouseopts = data.Mode5;
            _this.selectedHardware.ehouseopts2 = data.Mode6;
          } else if (data.Type.indexOf('Relay-Net') >= 0) {
            _this.selectedHardware.relaynetpollinputs = data.Mode1 === 1;
            _this.selectedHardware.relaynetpollrelays = data.Mode2 === 1;
            _this.selectedHardware.relaynetpollinterval = data.Mode3;
            _this.selectedHardware.relaynetinputcount = data.Mode4;
            _this.selectedHardware.relaynetrelaycount = data.Mode5;
          } else if (data.Type.indexOf('Philips Hue') >= 0) {
            _this.selectedHardware.tcpaddress = data.Address;
            _this.selectedHardware.tcpport = data.Port;
            _this.selectedHardware.username = data.Username;
            _this.selectedHardware.pollinterval = data.Mode1;
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.addgroups = (data.Mode2 & 1);
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.addscenes = (data.Mode2 & 2);
          } else if (data.Type.indexOf('Winddelen') >= 0) {
            _this.selectedHardware.millselect = data.Mode1;
            _this.selectedHardware.nrofwinddelen = data.Port;
          } else if (data.Type.indexOf('Honeywell') >= 0) {
            _this.selectedHardware.hwAccessToken = data.Username;
            _this.selectedHardware.hwRefreshToken = data.Password;
            const tmp = data.Extra;
            const tmparray = tmp.split('|');
            if (tmparray.length === 2) {
              _this.selectedHardware.hwApiKey = atob(tmparray[0]);
              _this.selectedHardware.hwApiSecret = atob(tmparray[1]);
            }
          } else if (data.Type.indexOf('Goodwe solar inverter via Web') >= 0) {
            _this.selectedHardware.serverselect = data.Mode1;
            _this.selectedHardware.username = data.Username;
          }
          if (data.Type.indexOf('MySensors Gateway with MQTT') >= 0) {

            // Break out any possible topic prefix pieces.
            const CAfilenameParts = data.Extra.split('#');

            // There should be 1 piece or 3 pieces.
            switch (CAfilenameParts.length) {
              case 2:
                console.log('MySensorsMQTT: Truncating CAfilename; Stray topic was present.');
                _this.selectedHardware.filename = CAfilenameParts[0];
                _this.selectedHardware.mqtttopicin = '';
                _this.selectedHardware.mqtttopicout = '';
                break;
              case 1:
              case 0:
                console.log('MySensorsMQTT: Only a CAfilename present.');
                _this.selectedHardware.filename = data.Extra;
                _this.selectedHardware.mqtttopicin = '';
                _this.selectedHardware.mqtttopicout = '';
                break;
              default:
                console.log('MySensorsMQTT: Stacked data in CAfilename present. Separating out topic prefixes.');
                _this.selectedHardware.filename = CAfilenameParts[0];
                _this.selectedHardware.mqtttopicin = CAfilenameParts[1];
                _this.selectedHardware.mqtttopicout = CAfilenameParts[2];
                break;
            }

            _this.selectedHardware.topicselect = data.Mode1;
            _this.selectedHardware.tlsversion = data.Mode2;
            _this.selectedHardware.preventloop = data.Mode3;
          } else if (data.Type.indexOf('MQTT') >= 0) {
            _this.selectedHardware.filename = data.Extra;
            _this.selectedHardware.topicselect = data.Mode1;
            _this.selectedHardware.tlsversion = data.Mode2;
            _this.selectedHardware.preventloop = data.Mode3;
          } else if (data.Type.indexOf('Rtl433') >= 0) {
            _this.selectedHardware.rtl433cmdline = data.Extra;
          }
          if (
            (data.Type.indexOf('Domoticz') >= 0) ||
            (data.Type.indexOf('ICY') >= 0) ||
            (data.Type.indexOf('Eco Devices') >= 0) ||
            (data.Type.indexOf('Toon') >= 0) ||
            (data.Type.indexOf('Atag') >= 0) ||
            (data.Type.indexOf('Nest Th') >= 0 && data.Type.indexOf('OAuth') === -1) ||
            (data.Type.indexOf('PVOutput') >= 0) ||
            (data.Type.indexOf('ETH8020') >= 0) ||
            (data.Type.indexOf('Daikin') >= 0) ||
            (data.Type.indexOf('Sterbox') >= 0) ||
            (data.Type.indexOf('Anna') >= 0) ||
            (data.Type.indexOf('KMTronic') >= 0) ||
            (data.Type.indexOf('MQTT') >= 0) ||
            (data.Type.indexOf('Netatmo') >= 0) ||
            (data.Type.indexOf('HTTP') >= 0) ||
            (data.Type.indexOf('Thermosmart') >= 0) ||
            (data.Type.indexOf('Tado') >= 0) ||
            (data.Type.indexOf('Tesla') >= 0) ||
            (data.Type.indexOf('Logitech Media Server') >= 0) ||
            (data.Type.indexOf('HEOS by DENON') >= 0) ||
            (data.Type.indexOf('Razberry') >= 0) ||
            (data.Type.indexOf('Comm5') >= 0)
          ) {
            _this.selectedHardware.username = data.Username;
            _this.selectedHardware.password = data.Password;
          }
          if (data.Type.indexOf('Evohome via Web') >= 0) {
            _this.selectedHardware.username = data.Username;
            _this.selectedHardware.password = data.Password;

            let Pollseconds = data.Mode1;
            if (Pollseconds < 10) {
              Pollseconds = 60;
            }
            _this.selectedHardware.updatefrequencyevohomeweb = Pollseconds;

            const UseFlags = data.Mode2;
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.disableautoevohomeweb = ((UseFlags & 1) ^ 1) === 0;
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.showscheduleevohomeweb = ((UseFlags & 2) >>> 1) === 0;
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.showlocationevohomeweb = ((UseFlags & 4) >>> 2) === 0;
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.evoprecision = (UseFlags & 24);

            const Location = data['Mode3'];
            _this.range10 = [];
            for (let i = 1; i < 10; i++) {
              _this.range10.push(i);
            }
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.comboevolocation = Location >>> 12;
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.comboevogateway = (Location >>> 8) & 15;
            // tslint:disable-next-line:no-bitwise
            _this.selectedHardware.comboevotcs = (Location >>> 4) & 15;
          }
        }
      }
    });
  }

  RegisterPhilipsHue() {
    const address = this.selectedHardware.tcpaddress;
    if (address === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
      return;
    }
    const port = this.selectedHardware.tcpport;
    if (port === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
      return;
    }
    const username = this.selectedHardware.username;
    this.hardwareService.registerHue(address, port, username)
      .subscribe((data: RegisterHueResponse) => {
        if (data.status === 'ERR') {
          this.notificationService.ShowNotify(data.statustext, 2500, true);
          return;
        }
        this.selectedHardware.username = data.username;
        this.notificationService.ShowNotify(this.translationService.t('Registration successful!'), 2500);
      }, error => {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Problem registrating with the Philips Hue bridge!'), 2500, true);
      });
  }

  onBaudratep1Change() {
    if (this.selectedHardware.baudratep1 === 0) {
      this.selectedHardware.disablecrcp1 = false;
      this.divcrcp1Display = false;
    } else {
      this.selectedHardware.disablecrcp1 = true;
      this.divcrcp1Display = true;
    }
  }

  onMethodChange() {
    if (this.selectedHardware.method === 0) {
      this.postdataDisplay = false;
    } else {
      this.postdataDisplay = true;
    }
  }

  AddHardware() {
    const name = this.selectedHardware.name;
    if (name === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
      return false;
    }

    const hardwaretype = this.selectedHardware.type;
    if (typeof hardwaretype === 'undefined') {
      this.notificationService.ShowNotify(this.translationService.t('Unknown device selected!'), 2500, true);
      return;
    }

    const bEnabled = this.selectedHardware.enabled;
    const datatimeout = this.selectedHardware.timeout;

    const text = this.typeLabel;

    // Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
    if (!$.isNumeric(hardwaretype)) {
      const selector = '#hardwarecontent #divpythonplugin #' + hardwaretype;
      let bIsOK = true;
      // FIXME: do it the Angular way
      // Make sure that all required fields have values
      const _this = this;
      $(selector + ' .text').each(function () {
        if ((typeof (this.attributes.required) !== 'undefined') && (this.value === '')) {
          $(selector + ' #' + this.id).focus();
          _this.notificationService.ShowNotify(_this.translationService.t('Please enter value for required field'), 2500, true);
          bIsOK = false;
        }
        return bIsOK;
      });
      if (bIsOK) {
        const username = ($(selector + ' #Username').length === 0) ? '' : $(selector + ' #Username').val();
        const password = ($(selector + ' #Password').length === 0) ? '' : $(selector + ' #Password').val();
        const address = ($(selector + ' #Address').length === 0) ? '' : $(selector + ' #Address').val();
        const port = ($(selector + ' #Port').length === 0) ? '' : $(selector + ' #Port').val();
        const serialport = ($(selector + ' #SerialPort').length === 0) ? '' : $(selector + ' #SerialPort').val();
        const Mode1 = ($(selector + ' #Mode1').length === 0) ? '' : $(selector + ' #Mode1').val();
        const Mode2 = ($(selector + ' #Mode2').length === 0) ? '' : $(selector + ' #Mode2').val();
        const Mode3 = ($(selector + ' #Mode3').length === 0) ? '' : $(selector + ' #Mode3').val();
        const Mode4 = ($(selector + ' #Mode4').length === 0) ? '' : $(selector + ' #Mode4').val();
        const Mode5 = ($(selector + ' #Mode5').length === 0) ? '' : $(selector + ' #Mode5').val();
        const Mode6 = ($(selector + ' #Mode6').length === 0) ? '' : $(selector + ' #Mode6').val();
        const extra = hardwaretype.toString();
        this.hardwareService.addHardware('94', bEnabled, datatimeout, name,
          Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, extra, username, password, address, port, serialport)
          .subscribe(() => {
            this.notificationService.ShowNotify(
              this.translationService.t('Hardware created, devices can be found in the devices tab!'), 2500);
            this.RefreshHardwareTable();
          }, error => {
            this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
          });
      }
      return;
    }
    if (text.indexOf('1-Wire') >= 0) {
      const owfspath = this.selectedHardware.owfspath;
      const oneWireSensorPollPeriod = String(this.selectedHardware.OneWireSensorPollPeriod);
      if (oneWireSensorPollPeriod === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a poll period for the sensors'), 2500, true);
        return;
      }
      const oneWireSwitchPollPeriod = String(this.selectedHardware.OneWireSwitchPollPeriod);
      if (oneWireSwitchPollPeriod === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a poll period for the switches'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(oneWireSensorPollPeriod)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a valid poll period for the sensors'), 2500, true);
        return;
      }

      if (!intRegex.test(oneWireSwitchPollPeriod)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a valid poll period for the switches'), 2500, true);
        return;
      }

      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name,
        oneWireSensorPollPeriod, oneWireSwitchPollPeriod, '', '', '', '', owfspath)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Rtl433 RTL-SDR receiver') >= 0) {
      const extra = this.selectedHardware.rtl433cmdline;
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name,
        '', '', '', '', '', '', extra)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('LAN') >= 0 &&
        text.indexOf('YouLess') === -1 &&
        text.indexOf('Denkovi') === -1 &&
        text.indexOf('ETH8020') === -1 &&
        text.indexOf('Daikin') === -1 &&
        text.indexOf('Sterbox') === -1 &&
        text.indexOf('Anna') === -1 &&
        text.indexOf('KMTronic') === -1 &&
        text.indexOf('MQTT') === -1 &&
        text.indexOf('Relay-Net') === -1 &&
        text.indexOf('Satel Integra') === -1 &&
        text.indexOf('eHouse') === -1 &&
        text.indexOf('Razberry') === -1 &&
        text.indexOf('MyHome OpenWebNet with LAN interface') === -1
      )
    ) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      let extra = '';
      if (text.indexOf('Evohome') >= 0) {
        extra = this.selectedHardware.controlleridevohometcp;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name,
        '', '', '', '', '', '', extra, undefined, undefined, address, port)
        .subscribe(() => {
          if ((bEnabled) && (text.indexOf('RFXCOM') >= 0)) {
            this.notificationService.ShowNotify(this.translationService.t('Please wait. Updating ....!'), 2500);
            timer(3000).subscribe(() => {
              this.hideAndRefreshHardwareTable();
            });
          } else {
            this.RefreshHardwareTable();
          }
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('Panasonic') >= 0) ||
      (text.indexOf('BleBox') >= 0) ||
      (text.indexOf('TE923') >= 0) ||
      (text.indexOf('Volcraft') >= 0) ||
      (text.indexOf('Dummy') >= 0) ||
      (text.indexOf('System Alive') >= 0) ||
      (text.indexOf('Kodi Media') >= 0) ||
      (text.indexOf('PiFace') >= 0) ||
      (text.indexOf('Evohome') >= 0 && text.indexOf('script') >= 0) ||
      (text.indexOf('Tellstick') >= 0) ||
      (text.indexOf('Motherboard') >= 0) ||
      (text.indexOf('YeeLight') >= 0) ||
      (text.indexOf('Arilux AL-LC0x') >= 0)
    ) {
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '')
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if ((text.indexOf('GPIO') >= 0) && (text.indexOf('sysfs GPIO') === -1)) {
      let gpiodebounce = String(this.selectedHardware.gpiodebounce);
      let gpioperiod = String(this.selectedHardware.gpioperiod);
      let gpiopollinterval = String(this.selectedHardware.gpiopollinterval);
      if (gpiodebounce === '') {
        gpiodebounce = '50';
      }
      if (gpioperiod === '') {
        gpioperiod = '50';
      }
      if (gpiopollinterval === '') {
        gpiopollinterval = '0';
      }
      const Mode1 = gpiodebounce;
      const Mode2 = gpioperiod;
      const Mode3 = gpiopollinterval;
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, Mode1, Mode2, Mode3, '', '', '')
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('sysfs GPIO') >= 0) {
      const Mode1 = this.selectedHardware.sysfsautoconfigure ? 1 : 0;
      const Mode2 = String(this.selectedHardware.sysfsdebounce);
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, Mode1.toString(), Mode2, '', '', '', '')
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('I2C ') >= 0) {
      const hardwaretype2 = this.selectedHardware.i2clocal;
      const i2cpath = this.selectedHardware.i2cpath;
      let i2caddress = '';
      let i2cinvert = '';

      const text1 = this.hardwareI2CTypes.find(_ => _.idx === hardwaretype2) ?
        this.hardwareI2CTypes.find(_ => _.idx === hardwaretype2).name : '';
      if (text1.indexOf('I2C sensor PIO 8bit expander PCF8574') >= 0) {
        i2caddress = this.selectedHardware.i2caddress;
        i2cinvert = this.selectedHardware.i2cinvert ? '1' : '0';
      } else if (text1.indexOf('I2C sensor GPIO 16bit expander MCP23017') >= 0) {
        i2caddress = this.selectedHardware.i2caddress;
        i2cinvert = this.selectedHardware.i2cinvert ? '1' : '0';
      }

      this.hardwareService.addHardware(hardwaretype2.toString(), bEnabled, datatimeout, name, i2cinvert, '', '', '', '', '',
        undefined, undefined, undefined, i2caddress, i2cpath)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding I2C hardware!'), 2500, true);
        });
    } else if (text.indexOf('USB') >= 0 || text.indexOf('Teleinfo EDF') >= 0) {
      let Mode1 = 0;
      let extra = '';
      let Mode2 = '';
      let Mode3 = '';
      const serialport = this.serialDevices.find(_ => _.value === this.selectedHardware.serialport) ?
        this.serialDevices.find(_ => _.value === this.selectedHardware.serialport).name : undefined;
      if (typeof serialport === 'undefined') {
        this.notificationService.ShowNotify(this.translationService.t('No serial port selected!'), 2500, true);
        return;
      }

      if (text.indexOf('Evohome') >= 0) {
        const baudrate = this.selectedHardware.baudrateevohome;
        extra = this.selectedHardware.controllerid;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
      }

      if (text.indexOf('MySensors') >= 0) {
        const baudrate = this.selectedHardware.baudrate;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
      }

      if (text.indexOf('P1 Smart Meter') >= 0) {
        const baudrate = this.selectedHardware.baudratep1;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
        Mode2 = this.selectedHardware.disablecrcp1 ? '0' : '1';
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '0';
        }
        Mode3 = ratelimitp1;
      }
      if (text.indexOf('Teleinfo EDF') >= 0) {
        const baudrate = this.selectedHardware.baudrateteleinfo;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
        Mode2 = this.selectedHardware.disablecrcp1 ? '0' : '1';
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '60';
        }
        Mode3 = ratelimitp1;
      }

      if (text.indexOf('USBtin') >= 0) {
        // Mode1 = $("#hardwarecontent #divusbtin #combotypecanusbtin option:selected").val();
        const ActivateMultiblocV8 = this.selectedHardware.activateMultiblocV8 ? 1 : 0;
        const ActivateCanFree = this.selectedHardware.activateCanFree ? 1 : 0;
        const DebugActiv = this.selectedHardware.debugusbtin;
        // tslint:disable-next-line:no-bitwise
        Mode1 = (ActivateCanFree & 0x01);
        // tslint:disable-next-line:no-bitwise
        Mode1 <<= 1;
        // tslint:disable-next-line:no-bitwise
        Mode1 += (ActivateMultiblocV8 & 0x01);
        Mode2 = DebugActiv.toString();
      }

      if (text.indexOf('Denkovi') >= 0) {
        Mode1 = this.selectedHardware.modeldenkoviusbdevices;
      }

      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name,
        Mode1.toString(), '', '', '', '', '', extra, undefined,
        undefined, undefined, serialport)
        .subscribe(() => {
          if ((bEnabled) && (text.indexOf('RFXCOM') >= 0)) {
            this.notificationService.ShowNotify(this.translationService.t('Please wait. Updating ....!'), 2500);
            timer(3000).subscribe(() => {
              this.hideAndRefreshHardwareTable();
            });
          } else {
            this.RefreshHardwareTable();
          }
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('LAN') >= 0 &&
        text.indexOf('YouLess') === -1 &&
        text.indexOf('Denkovi') === -1 &&
        text.indexOf('ETH8020') === -1 &&
        text.indexOf('Daikin') === -1 &&
        text.indexOf('Sterbox') === -1 &&
        text.indexOf('Anna') === -1 &&
        text.indexOf('KMTronic') === -1 &&
        text.indexOf('MQTT') === -1 &&
        text.indexOf('Relay-Net') === -1 &&
        text.indexOf('Satel Integra') === -1 &&
        text.indexOf('eHouse') === -1 &&
        text.indexOf('Razberry') === -1 &&
        text.indexOf('MyHome OpenWebNet with LAN interface') === -1
      )
    ) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      let extra = '';
      if (text.indexOf('Evohome') >= 0) {
        extra = this.selectedHardware.controlleridevohometcp;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        extra, undefined, undefined, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('LAN') >= 0 && ((text.indexOf('YouLess') >= 0) || (text.indexOf('Denkovi') >= 0))) ||
      (text.indexOf('Relay-Net') >= 0) || (text.indexOf('Satel Integra') >= 0) || (text.indexOf('eHouse') >= 0) ||
      (text.indexOf('Harmony') >= 0) || (text.indexOf('Xiaomi Gateway') >= 0) ||
      (text.indexOf('MyHome OpenWebNet with LAN interface') >= 0)
    ) {
      let Mode1 = '0';
      let Mode2 = '0';
      let Mode3 = '0';
      let Mode4 = '0';
      let Mode5 = '0';

      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      if (text.indexOf('Satel Integra') >= 0) {
        const pollinterval = String(this.selectedHardware.pollinterval);
        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }
        Mode1 = pollinterval;
      } else if (text.indexOf('Denkovi') >= 0) {
        const pollinterval = String(this.selectedHardware.pollinterval);
        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }
        Mode1 = pollinterval;
        if (text.indexOf('Modules with LAN (HTTP)') >= 0) {
          Mode2 = String(this.selectedHardware.modeldenkovidevices);
        } else if (text.indexOf('Modules with LAN (TCP)') >= 0) {
          Mode2 = String(this.selectedHardware.modeldenkovitcpdevices);
          Mode3 = String(this.selectedHardware.denkovislaveid);
          if (Mode2 === '1') {
            // const intRegex = /^\d+$/;
            if (isNaN(<any>Mode3) || Number(Mode3) < 1 || Number(Mode3) > 247) {
              this.notificationService.ShowNotify(this.translationService.t('Invalid Slave ID! Enter value from 1 to 247!'), 2500, true);
              return;
            }
          } else {
            Mode3 = '0';
          }
        }
      } else if (text.indexOf('eHouse') >= 0) {
        const pollinterval = String(this.selectedHardware.pollinterval);
        Mode2 = this.selectedHardware.ehouseautodiscovery ? '1' : '0';
        Mode3 = this.selectedHardware.ehouseaddalarmin ? '1' : '0';
        Mode4 = this.selectedHardware.ehouseprodiscovery ? '1' : '0';
        Mode5 = String(this.selectedHardware.ehouseopts);
        const Mode6 = this.selectedHardware.ehouseopts2;
        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }
        Mode1 = pollinterval;
      } else if (text.indexOf('Relay-Net') >= 0) {
        Mode1 = this.selectedHardware.relaynetpollinputs ? '1' : '0';
        Mode2 = this.selectedHardware.relaynetpollrelays ? '1' : '0';
        const pollinterval = String(this.selectedHardware.relaynetpollinterval);
        const inputcount = String(this.selectedHardware.relaynetinputcount);
        const relaycount = String(this.selectedHardware.relaynetrelaycount);

        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }

        if (inputcount === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter input count!'), 2500, true);
          return;
        }

        if (relaycount === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter relay count!'), 2500, true);
          return;
        }

        Mode3 = pollinterval;
        Mode4 = inputcount;
        Mode5 = relaycount;
      }
      const password: any = this.selectedHardware.password;
      if (text.indexOf('MyHome OpenWebNet with LAN interface') >= 0) {
        if (password !== '') {
          if ((password.length < 5) || (password.length > 16)) {
            this.notificationService.ShowNotify(
              this.translationService.t('Please enter a password between 5 and 16 characters'), 2500, true);
            return;
          }

          // const intRegex3 = /^[a-zA-Z0-9]*$/;
          // if (!intRegex3.test(password)) {
          //   this.notificationService.ShowNotify(this.translationService.t('Please enter a numeric or alphanumeric (for HMAC) password'),
          //     2500, true);
          //   return;
          // }
        }
        const ratelimitp1 = this.selectedHardware.ratelimitp1;
        if ((String(ratelimitp1) === '') || (isNaN(ratelimitp1))) {
          this.notificationService.ShowNotify(this.translationService.t('Please enter rate limit!'), 2500, true);
          return;
        }
        Mode1 = String(ratelimitp1);
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, Mode1, Mode2, Mode3, Mode4, Mode5,
        '', undefined, undefined, password, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Philips Hue') >= 0) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;

      if (username === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a username!'), 2500, true);
        return;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        '', username, undefined, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('Domoticz') >= 0) ||
      (text.indexOf('Eco Devices') >= 0) ||
      (text.indexOf('ETH8020') >= 0) ||
      (text.indexOf('Daikin') >= 0) ||
      (text.indexOf('Sterbox') >= 0) ||
      (text.indexOf('Anna') >= 0) ||
      (text.indexOf('KMTronic') >= 0) ||
      (text.indexOf('MQTT') >= 0) ||
      (text.indexOf('Logitech Media Server') >= 0) ||
      (text.indexOf('HEOS by DENON') >= 0) ||
      (text.indexOf('Razberry') >= 0)
    ) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      let extra = '';
      let Mode1 = '';
      let Mode2 = '';
      let Mode3 = '';
      if (text.indexOf('MySensors Gateway with MQTT') >= 0) {
        extra = this.selectedHardware.filename;
        Mode1 = String(this.selectedHardware.topicselect);
        Mode2 = String(this.selectedHardware.tlsversion);
        Mode3 = String(this.selectedHardware.preventloop);

        if (this.selectedHardware.filename.indexOf('#') >= 0) {
          this.notificationService.ShowNotify(this.translationService.t('CA Filename cannot contain a "#" symbol!'), 2500, true);
          return;
        }
        if (this.selectedHardware.mqtttopicin.indexOf('#') >= 0) {
          this.notificationService.ShowNotify(this.translationService.t('Publish Prefix cannot contain a "#" symbol!'), 2500, true);
          return;
        }
        if (this.selectedHardware.mqtttopicout.indexOf('#') >= 0) {
          this.notificationService.ShowNotify(this.translationService.t('Subscribe Prefix cannot contain a "#" symbol!'), 2500, true);
          return;
        }
        if ((Mode1 === '2') && ((this.selectedHardware.mqtttopicin === '') || (this.selectedHardware.mqtttopicout === ''))) {
          this.notificationService.ShowNotify(this.translationService.t('Please enter Topic Prefixes!'), 2500, true);
          return;
        }
        if (Mode1 === '2') {
          if ((this.selectedHardware.mqtttopicin === '') || (this.selectedHardware.mqtttopicout === '')) {
            this.notificationService.ShowNotify(this.translationService.t('Please enter Topic Prefixes!'), 2500, true);
            return;
          }
          extra += '#';
          extra += this.selectedHardware.mqtttopicin;
          extra += '#';
          extra += this.selectedHardware.mqtttopicout;
        }
      } else if (text.indexOf('MQTT') >= 0) {
        extra = this.selectedHardware.filename;
        Mode1 = String(this.selectedHardware.topicselect);
        Mode2 = String(this.selectedHardware.tlsversion);
        Mode3 = String(this.selectedHardware.preventloop);
      }
      if (text.indexOf('Eco Devices') >= 0) {
        Mode1 = String(this.selectedHardware.modelecodevices);
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '60';
        }
        Mode2 = ratelimitp1;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, Mode1, Mode2, Mode3, '', '', '',
        extra, username, password, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if ((text.indexOf('Underground') >= 0) || (text.indexOf('DarkSky') >= 0) || (text.indexOf('AccuWeather') >= 0) ||
      (text.indexOf('Open Weather Map') >= 0)) {
      const apikey = this.selectedHardware.apikey;
      if (apikey === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an API Key!'), 2500, true);
        return;
      }
      const location = this.selectedHardware.location;
      if (location === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Location!'), 2500, true);
        return;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        undefined, apikey, location)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Buienradar') >= 0) {
      let timeframe = this.selectedHardware.timeframe;
      if (timeframe === 0) {
        timeframe = 30;
      }
      let threshold = this.selectedHardware.threshold;
      if (threshold === 0) {
        threshold = 25;
      }
      const location = this.selectedHardware.location;
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, timeframe.toString(), threshold.toString(),
        '', '', '', '', undefined, undefined, location)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if ((text.indexOf('HTTP/HTTPS') >= 0)) {
      const url = this.selectedHardware.url;
      if (url === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an url!'), 2500, true);
        return;
      }
      const script = this.selectedHardware.script;
      if (script === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a script!'), 2500, true);
        return;
      }
      const refresh = String(this.selectedHardware.refresh);
      if (refresh === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a refresh rate!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      const method = this.selectedHardware.method;
      if (typeof method === 'undefined') {
        this.notificationService.ShowNotify(this.translationService.t('No HTTP method selected!'), 2500, true);
        return;
      }
      const contenttype = this.selectedHardware.contenttype;
      const headers = this.selectedHardware.headers;
      const postdata = this.selectedHardware.postdata;

      let extra = btoa(script) + '|' + btoa(String(method)) + '|' + btoa(contenttype) + '|' + btoa(headers);
      if (method === 1) {
        extra = extra + '|' + btoa(postdata);
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        extra, username, password, url, refresh)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('SBFSpot') >= 0) {
      const configlocation = this.selectedHardware.location;
      if (configlocation === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Location!'), 2500, true);
        return;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        undefined, configlocation)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Winddelen') >= 0) {
      const mill_id = this.selectedHardware.millselect;
      const mill_name = this.millselectOptions.find(_ => _.value === mill_id).label;
      const nrofwinddelen = this.selectedHardware.nrofwinddelen;
      const intRegex = /^\d+$/;
      if (!intRegex.test(nrofwinddelen)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Number!'), 2500, true);
        return;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, mill_id.toString(), '', '', '', '', '',
        undefined, undefined, undefined, mill_name, nrofwinddelen)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Honeywell') >= 0) {
      const apiKey = this.selectedHardware.hwApiKey;
      const apiSecret = this.selectedHardware.hwApiSecret;
      const accessToken = this.selectedHardware.hwAccessToken;
      const refreshToken = this.selectedHardware.hwRefreshToken;
      const extra = btoa(apiKey) + '|' + btoa(apiSecret);

      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        extra, accessToken, refreshToken)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('SolarEdge via Web') >= 0) {
      const apikey = this.selectedHardware.apikey;
      if (apikey === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an API Key!'), 2500, true);
        return;
      }
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        undefined, apikey)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Nest Th') >= 0 && text.indexOf('OAuth') >= 0) {
      const apikey = this.selectedHardware.apikey;
      const productid = this.selectedHardware.productid;
      const productsecret = this.selectedHardware.productsecret;
      const productpin = this.selectedHardware.productpin;

      if (apikey === '' && (productid === '' || productsecret === '' || productpin === '')) {
        this.notificationService.ShowNotify(
          this.translationService.t('Please enter an API Key or a combination of Product Id, Product Secret and PIN!'), 2500, true);
        return;
      }

      const extra = btoa(productid) + '|' + btoa(productsecret) + '|' + btoa(productpin);
      // console.log('Updating extra2: ' + extra);

      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        extra, apikey)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Toon') >= 0) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      const agreement = String(this.selectedHardware.agreement);
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, agreement, '', '', '', '', '',
        undefined, username, password)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Tesla') >= 0) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      const vinnr = this.selectedHardware.teslavinnr;
      let activeinterval = Number(this.selectedHardware.teslaactiveinterval);
      if (activeinterval < 1) {
        activeinterval = 1;
      }
      let defaultinterval = Number(this.selectedHardware.tesladefaultinterval);
      if (defaultinterval < 1) {
        defaultinterval = 20;
      }
      const allowwakeup = Number(this.selectedHardware.teslaallowwakeup);
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, defaultinterval.toString(), activeinterval.toString(), allowwakeup.toString(), '', '', '',
        vinnr, username, password)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('ICY') >= 0) ||
      (text.indexOf('Atag') >= 0) ||
      (text.indexOf('Nest Th') >= 0 && text.indexOf('OAuth') === -1) ||
      (text.indexOf('PVOutput') >= 0) ||
      (text.indexOf('Netatmo') >= 0) ||
      (text.indexOf('Thermosmart') >= 0) ||
      (text.indexOf('Tado') >= 0) ||
      (text.indexOf('HTTP') >= 0)
    ) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, '', '', '', '', '', '',
        undefined, username, password)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Goodwe solar inverter via Web') >= 0) {
      const username = this.selectedHardware.username;

      if (username === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter your Goodwe username!'), 2500, true);
        return;
      }
      const Mode1 = String(this.selectedHardware.serverselect);

      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name, Mode1, '', '', '', '', '',
        undefined, username)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    } else if (text.indexOf('Evohome via Web') >= 0) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      let Pollseconds = this.selectedHardware.updatefrequencyevohomeweb;
      if (Pollseconds < 10) {
        Pollseconds = 60;
      }

      let UseFlags = 0;
      if (this.selectedHardware.showlocationevohomeweb) {
        this.selectedHardware.showlocationevohomeweb = true;
        // tslint:disable-next-line:no-bitwise
        UseFlags = UseFlags | 4;
      }

      if (!this.selectedHardware.disableautoevohomeweb) { // reverted value - default 0 is true
        // tslint:disable-next-line:no-bitwise
        UseFlags = UseFlags | 1;
      }

      if (this.selectedHardware.showscheduleevohomeweb) {
        // tslint:disable-next-line:no-bitwise
        UseFlags = UseFlags | 2;
      }

      const Precision = this.selectedHardware.evoprecision;
      UseFlags += Precision;

      let evo_installation = this.selectedHardware.comboevolocation * 4096;
      evo_installation = evo_installation + this.selectedHardware.comboevogateway * 256;
      evo_installation = evo_installation + this.selectedHardware.comboevotcs * 16;

      this.hardwareService.addHardware(hardwaretype.toString(), bEnabled, datatimeout, name,
        Pollseconds.toString(), UseFlags.toString(), evo_installation.toString(), '', '', '',
        undefined, username, password)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
        });
    }
  }

  UpdateHardwareParamControls() {
    this.enabledDisabled = false;
    this.hardwarenameDisabled = false;
    this.typeDisabled = false;
    this.timeoutDisabled = false;

    const text = this.typeLabel;

    this.usernameDisplay = true;
    this.divevohomeDisplay = false;
    this.divevohometcpDisplay = false;
    this.divevohomewebDisplay = false;
    this.divusbtinDisplay = false;
    this.divbaudratemysensorsDisplay = false;
    this.divbaudratep1Display = false;
    this.divbaudrateteleinfoDisplay = false;
    this.divmodelecodevicesDisplay = false;
    this.divcrcp1Display = false;
    this.divratelimitp1Display = false;
    this.divlocationDisplay = false;
    this.divphilipshueDisplay = false;
    this.divwinddelenDisplay = false;
    this.divhoneywellDisplay = false;
    this.divmqttDisplay = false;
    this.divmysensorsmqttDisplay = false;
    this.divsolaredgeapiDisplay = false;
    this.divnestoauthapiDisplay = false;
    this.divenecotoonDisplay = false;
    this.divteslaDisplay = false;
    this.div1wireDisplay = false;
    this.divgoodwewebDisplay = false;
    this.divi2clocalDisplay = false;
    this.divi2caddressDisplay = false;
    this.divi2cinvertDisplay = false;
    this.divpollintervalDisplay = false;
    this.divpythonpluginDisplay = false;
    this.divrelaynetDisplay = false;
    this.divgpioDisplay = false;
    this.divsysfsgpioDisplay = false;
    this.divmodeldenkovidevicesDisplay = false;
    this.divmodeldenkoviusbdevicesDisplay = false;
    this.divmodeldenkovitcpdevicesDisplay = false;

    this.divundergroundDisplay = false;
    this.divbuienradarDisplay = false;
    this.divserialDisplay = false;
    this.divremoteDisplay = false;
    this.divloginDisplay = false;
    this.divhttppollerDisplay = false;

    // Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
    // Python Plugins have the plugin name, not the hardware type id, as the value
    if (!$.isNumeric(this.selectedHardware.type)) {
      this.divpythonpluginDisplay = true;
      return;
    }

    if (text.indexOf('eHouse') >= 0) {
      this.divehouseDisplay = true;
    } else if (text.indexOf('I2C ') >= 0) {
      this.divi2clocalDisplay = true;
      this.divi2caddressDisplay = false;
      this.divi2cinvertDisplay = false;
      const text1 = this.hardwareI2CTypes.find(_ => _.idx === this.selectedHardware.i2clocal).name;
      if (text1.indexOf('I2C sensor PIO 8bit expander PCF8574') >= 0) {
        this.divi2caddressDisplay = true;
        this.divi2cinvertDisplay = true;
      } else if (text1.indexOf('I2C sensor GPIO 16bit expander MCP23017') >= 0) {
        this.divi2caddressDisplay = true;
        this.divi2cinvertDisplay = true;
      }
    } else if ((text.indexOf('GPIO') >= 0) && (text.indexOf('sysfs GPIO') === -1)) {
      this.divgpioDisplay = true;
    } else if (text.indexOf('sysfs GPIO') >= 0) {
      this.divsysfsgpioDisplay = true;
      this.divserialDisplay = false;
      this.divremoteDisplay = false;
      this.divloginDisplay = false;
      this.divundergroundDisplay = false;
      this.divhttppollerDisplay = false;
    } else if (text.indexOf('USB') >= 0 || text.indexOf('Teleinfo EDF') >= 0) {
      if (text.indexOf('Evohome') >= 0) {
        this.divevohomeDisplay = true;
      }
      if (text.indexOf('MySensors') >= 0) {
        this.divbaudratemysensorsDisplay = true;
      }
      if (text.indexOf('P1 Smart Meter') >= 0) {
        this.divbaudratep1Display = true;
        this.divratelimitp1Display = true;
        this.divcrcp1Display = true;
      }
      if (text.indexOf('Teleinfo EDF') >= 0) {
        this.divbaudrateteleinfoDisplay = true;
        this.divratelimitp1Display = true;
        this.divcrcp1Display = true;
      }
      if (text.indexOf('USBtin') >= 0) {
        this.divusbtinDisplay = true;
      }
      if (text.indexOf('Denkovi') >= 0) {
        this.divmodeldenkoviusbdevicesDisplay = true;
      }
      this.divserialDisplay = true;
    } else if ((text.indexOf('LAN') >= 0 || text.indexOf('Harmony') >= 0 || text.indexOf('Eco Devices') >= 0 ||
      text.indexOf('MySensors Gateway with MQTT') >= 0) && text.indexOf('YouLess') === -1 &&
      text.indexOf('Denkovi') === -1 && text.indexOf('Relay-Net') === -1 && text.indexOf('Satel Integra') === -1 &&
      text.indexOf('eHouse') === -1 && text.indexOf('MyHome OpenWebNet with LAN interface') === -1) {
      this.divremoteDisplay = true;
      if (text.indexOf('Eco Devices') >= 0) {
        this.divmodelecodevicesDisplay = true;
        this.divratelimitp1Display = true;
        this.divloginDisplay = true;
      }
      if (text.indexOf('P1 Smart Meter') >= 0) {
        this.divratelimitp1Display = true;
        this.divcrcp1Display = true;
      }
      if (text.indexOf('Evohome') >= 0) {
        this.divevohometcpDisplay = true;
      }
    } else if ((text.indexOf('LAN') >= 0 || text.indexOf('MySensors Gateway with MQTT') >= 0) && (text.indexOf('YouLess') >= 0 ||
      text.indexOf('Denkovi') >= 0 || text.indexOf('Relay-Net') >= 0 || text.indexOf('Satel Integra') >= 0) ||
      text.indexOf('eHouse') >= 0 || (text.indexOf('Xiaomi Gateway') >= 0) || text.indexOf('MyHome OpenWebNet with LAN interface') >= 0) {
      this.divremoteDisplay = true;
      this.divloginDisplay = true;

      if (text.indexOf('Relay-Net') >= 0) {
        this.usernameDisplay = true;
        this.passwordDisplay = true;
        this.divrelaynetDisplay = true;
      } else if (text.indexOf('Satel Integra') >= 0) {
        this.divpollintervalDisplay = true;
        this.selectedHardware.pollinterval = 1000;
      } else if (text.indexOf('eHouse') >= 0) {
        this.divpollintervalDisplay = true;
        this.selectedHardware.pollinterval = 1000;
        // this.passwordDisplay = true;
        this.divehouseDisplay = true;
      } else if (text.indexOf('MyHome OpenWebNet with LAN interface') >= 0) {
        this.divratelimitp1Display = true;
        this.selectedHardware.tcpport = '20000';
        // $("#hardwarecontent #divratelimitp1").val(300);
      } else if (text.indexOf('Denkovi') >= 0) {
        this.divpollintervalDisplay = true;
        this.selectedHardware.pollinterval = 10000;
        if (text.indexOf('Modules with LAN (HTTP)') >= 0) {
          this.divmodeldenkovidevicesDisplay = true;
        } else if (text.indexOf('Modules with LAN (TCP)') >= 0) {
          this.divmodeldenkovitcpdevicesDisplay = true;
          const board = this.selectedHardware.modeldenkovitcpdevices;
          if (board === 0) {
            this.denkovislaveidDisplay = false;
          } else {
            this.denkovislaveidDisplay = true;
          }
        }
        this.usernameDisplay = false;
      }
    } else if (text.indexOf('Domoticz') >= 0) {
      this.divremoteDisplay = true;
      this.divloginDisplay = true;
      this.selectedHardware.tcpport = '6144';
    } else if (text.indexOf('SolarEdge via') >= 0) {
      this.divsolaredgeapiDisplay = true;
    } else if (text.indexOf('Nest Th') >= 0 && text.indexOf('OAuth') >= 0) {
      this.divnestoauthapiDisplay = true;
    } else if (text.indexOf('Toon') >= 0) {
      this.divloginDisplay = true;
      this.divenecotoonDisplay = true;
    } else if (text.indexOf('Tesla') >= 0) {
      this.divloginDisplay = true;
      this.divteslaDisplay = true;
    } else if (text.indexOf('SBFSpot') >= 0) {
      this.divlocationDisplay = true;
    } else if (
      (text.indexOf('ICY') >= 0) ||
      (text.indexOf('Atag') >= 0) ||
      (text.indexOf('Nest Th') >= 0 && text.indexOf('OAuth') === -1) ||
      (text.indexOf('PVOutput') >= 0) ||
      (text.indexOf('Netatmo') >= 0) ||
      (text.indexOf('Thermosmart') >= 0) ||
      (text.indexOf('Tado') >= 0)
    ) {
      this.divloginDisplay = true;
    } else if (text.indexOf('HTTP') >= 0) {
      this.divloginDisplay = true;
      this.divhttppollerDisplay = true;

      const method = this.selectedHardware.method;
      if (method === 0) {
        this.postdataDisplay = false;
      } else {
        this.postdataDisplay = true;
      }
    } else if ((text.indexOf('Underground') >= 0) || (text.indexOf('DarkSky') >= 0) || (text.indexOf('AccuWeather') >= 0) ||
      (text.indexOf('Open Weather Map') >= 0)) {
      this.divundergroundDisplay = true;
    } else if (text.indexOf('Buienradar') >= 0) {
      this.divbuienradarDisplay = true;
    } else if (text.indexOf('Philips Hue') >= 0) {
      this.divremoteDisplay = true;
      this.divphilipshueDisplay = true;
    } else if (text.indexOf('Yeelight') >= 0) {
    } else if (text.indexOf('Arilux AL-LC0x') >= 0) {
    } else if (text.indexOf('Winddelen') >= 0) {
      this.divwinddelenDisplay = true;
    } else if (text.indexOf('Honeywell') >= 0) {
      this.divhoneywellDisplay = true;
    } else if (text.indexOf('Logitech Media Server') >= 0) {
      this.divremoteDisplay = true;
      this.divloginDisplay = true;
      this.selectedHardware.tcpport = '9000';
    } else if (text.indexOf('HEOS by DENON') >= 0) {
      this.divremoteDisplay = true;
      this.divloginDisplay = true;
      this.selectedHardware.tcpport = '1255';
    } else if (text.indexOf('MyHome OpenWebNet') >= 0) {
      this.divremoteDisplay = true;
      this.selectedHardware.tcpport = '20000';
    } else if (text.indexOf('1-Wire') >= 0) {
      this.div1wireDisplay = true;
    } else if (text.indexOf('Goodwe solar inverter via Web') >= 0) {
      this.divgoodwewebDisplay = true;
    } else if (text.indexOf('Evohome via Web') >= 0) {
      this.divevohomewebDisplay = true;
      this.divloginDisplay = true;
    }

    if (
      (text.indexOf('ETH8020') >= 0) ||
      (text.indexOf('Daikin') >= 0) ||
      (text.indexOf('Sterbox') >= 0) ||
      (text.indexOf('Anna') >= 0) ||
      (text.indexOf('MQTT') >= 0) ||
      (text.indexOf('KMTronic Gateway with LAN') >= 0) ||
      (text.indexOf('Razberry') >= 0)
    ) {
      this.divloginDisplay = true;
    }

    if (text.indexOf('Rtl433') >= 0) {
      this.divrtl433Display = true;
    } else {
      this.divrtl433Display = false;
    }

    if (text.indexOf('MySensors Gateway with MQTT') >= 0) {
      this.divmysensorsmqttDisplay = true;
    } else if (text.indexOf('MQTT') >= 0) {
      this.divmqttDisplay = true;
      if (
        (text.indexOf('The Things Network (MQTT') >= 0)
        || (text.indexOf('OctoPrint') >= 0)
      ) {
        this.mqtt_publishDisplay = false;
      } else {
        this.mqtt_publishDisplay = true;
      }
    }
  }

  onUpdateHardware() {
    if (this.selectedRow) {
      const data = this.selectedRow;
      const idx = data.DT_RowId;
      if (data.Type !== 'PLUGIN') { // Plugins can have non-numeric Mode data
        this.UpdateHardware(idx, data.Mode1, data.Mode2, data.Mode3, data.Mode4, data.Mode5, data.Mode6);
      } else {
        this.UpdateHardware(idx, data.Mode1, data.Mode2, data.Mode3, data.Mode4, data.Mode5, data.Mode6);
      }
    }
  }

  onDeleteHardware() {
    if (this.selectedRow) {
      const data = this.selectedRow;
      const idx = data.DT_RowId;
      this.DeleteHardware(idx);
    }
  }

  private DeleteHardware(idx) {
    const msg = this.translationService.t('Are you sure to delete this Hardware?\n\nThis action can not be undone...\n' +
      'All Devices attached will be removed!');
    bootbox.confirm(msg, (result) => {
      if (result === true) {
        this.hardwareService.deleteHardware(idx).subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem deleting hardware!'), 2500, true);
        });
      }
    });
  }

  private UpdateHardware(idx, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
    const name = this.selectedHardware.name;
    if (name === '') {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Name!'), 2500, true);
      return;
    }

    let hardwaretype = this.selectedHardware.type;
    if (typeof hardwaretype === 'undefined') {
      this.notificationService.ShowNotify(this.translationService.t('Unknown device selected!'), 2500, true);
      return;
    }

    const bEnabled = this.selectedHardware.enabled;

    const datatimeout = this.selectedHardware.timeout;

    const text = this.typeLabel;

    const _this = this;

    // Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
    if (!$.isNumeric(hardwaretype)) {
      const selector = '#hardwarecontent #divpythonplugin #' + hardwaretype;
      let bIsOK = true;
      // Make sure that all required fields have values
      // FIXME use Angular for this
      $(selector + ' .text').each(function () {
        if ((typeof (this.attributes.required) !== 'undefined') && (this.value === '')) {
          $(selector + ' #' + this.id).focus();
          _this.notificationService.ShowNotify(_this.translationService.t('Please enter value for required field'), 2500, true);
          bIsOK = false;
        }
        return bIsOK;
      });
      if (bIsOK) {
        // FIXME use Angular
        const username = ($(selector + ' #Username').length === 0) ? '' : $(selector + ' #Username').val();
        const password = ($(selector + ' #Password').length === 0) ? '' : $(selector + ' #Password').val();
        const address = ($(selector + ' #Address').length === 0) ? '' : $(selector + ' #Address').val();
        const port = ($(selector + ' #Port').length === 0) ? '' : $(selector + ' #Port').val();
        const serialport = ($(selector + ' #SerialPort').length === 0) ? '' : $(selector + ' #SerialPort').val();
        const Mode1Param = ($(selector + ' #Mode1').length === 0) ? '' : $(selector + ' #Mode1').val();
        const Mode2Param = ($(selector + ' #Mode2').length === 0) ? '' : $(selector + ' #Mode2').val();
        const Mode3Param = ($(selector + ' #Mode3').length === 0) ? '' : $(selector + ' #Mode3').val();
        const Mode4Param = ($(selector + ' #Mode4').length === 0) ? '' : $(selector + ' #Mode4').val();
        const Mode5Param = ($(selector + ' #Mode5').length === 0) ? '' : $(selector + ' #Mode5').val();
        const Mode6Param = ($(selector + ' #Mode6').length === 0) ? '' : $(selector + ' #Mode6').val();

        this.hardwareService.updateHardware(94, idx, name,
          Mode1Param, Mode2Param, Mode3Param, Mode4Param, Mode5Param, Mode6Param, hardwaretype.toString(), bEnabled, datatimeout,
          username, password, address, port, serialport)
          .subscribe(() => {
            this.RefreshHardwareTable();
          }, error => {
            this.notificationService.ShowNotify(this.translationService.t('Problem adding hardware!'), 2500, true);
          });
      }
      return;
    }

    if (text.indexOf('1-Wire') >= 0) {
      const extra = this.selectedHardware.owfspath;
      const Mode1Param = this.selectedHardware.OneWireSensorPollPeriod;
      const Mode2Param = this.selectedHardware.OneWireSwitchPollPeriod;
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name,
        Mode1Param.toString(), Mode2Param.toString(), Mode3, Mode4, Mode5, Mode6, extra, bEnabled, datatimeout)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('Panasonic') >= 0) ||
      (text.indexOf('BleBox') >= 0) ||
      (text.indexOf('TE923') >= 0) ||
      (text.indexOf('Volcraft') >= 0) ||
      (text.indexOf('GPIO') >= 0) ||
      (text.indexOf('Dummy') >= 0) ||
      (text.indexOf('System Alive') >= 0) ||
      (text.indexOf('PiFace') >= 0) ||
      (text.indexOf('I2C ') >= 0) ||
      (text.indexOf('Motherboard') >= 0) ||
      (text.indexOf('Kodi Media') >= 0) ||
      (text.indexOf('Evohome') >= 0 && text.indexOf('script') >= 0) ||
      (text.indexOf('YeeLight') >= 0) ||
      (text.indexOf('Arilux AL-LC0x') >= 0) ||
      (text.indexOf('sysfs GPIO') >= 0)
    ) {
      // if hardwaretype == 1000 => I2C sensors grouping
      if (hardwaretype === 1000) {
        hardwaretype = this.selectedHardware.i2clocal;
      }
      const text1 = this.selectedHardware.i2clocal ?
        this.hardwareI2CTypes.find(_ => _.idx === this.selectedHardware.i2clocal).name : undefined;
      let i2cpath, i2caddress, i2cinvert;
      if (text1.indexOf('I2C sensor') >= 0) {
        i2cpath = this.selectedHardware.i2cpath;
        i2caddress = '';
        i2cinvert = '';
        Mode1 = '';
      }
      if (text1.indexOf('I2C sensor PIO 8bit expander PCF8574') >= 0) {
        i2caddress = this.selectedHardware.i2caddress;
        i2cinvert = this.selectedHardware.i2cinvert ? 1 : 0;
        Mode1 = i2cinvert;
      } else if (text1.indexOf('I2C sensor GPIO 16bit expander MCP23017') >= 0) {
        i2caddress = this.selectedHardware.i2caddress;
        i2cinvert = this.selectedHardware.i2caddress ? 1 : 0;
        Mode1 = i2cinvert;
      }
      if ((text.indexOf('GPIO') >= 0) && (text.indexOf('sysfs GPIO') === -1)) {
        let gpiodebounce = String(this.selectedHardware.gpiodebounce);
        let gpioperiod = String(this.selectedHardware.gpioperiod);
        let gpiopollinterval = String(this.selectedHardware.gpiopollinterval);
        if (gpiodebounce === '') {
          gpiodebounce = '50';
        }
        if (gpioperiod === '') {
          gpioperiod = '50';
        }
        if (gpiopollinterval === '') {
          gpiopollinterval = '0';
        }
        Mode1 = gpiodebounce;
        Mode2 = gpioperiod;
        Mode3 = gpiopollinterval;
      }
      if (text.indexOf('sysfs GPIO') >= 0) {
        Mode1 = this.selectedHardware.sysfsautoconfigure ? 1 : 0;
        Mode2 = this.selectedHardware.sysfsdebounce;
      }
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout,
        undefined, undefined, i2caddress, i2cpath, undefined)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('USB') >= 0 || text.indexOf('Teleinfo EDF') >= 0) {
      Mode1 = '0';
      let serialport = this.selectedHardware.serialport ?
        this.serialDevices.find(_ => _.value === this.selectedHardware.serialport).name : undefined;
      if (typeof serialport === 'undefined') {
        if (bEnabled === true) {
          this.notificationService.ShowNotify(this.translationService.t('No serial port selected!'), 2500, true);
          return;
        } else {
          serialport = '';
        }
      }

      let extra = '';
      if (text.indexOf('Evohome') >= 0) {
        const baudrate = this.selectedHardware.baudrateevohome;
        extra = this.selectedHardware.controllerid;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
      }

      if (text.indexOf('MySensors') >= 0) {
        const baudrate = this.selectedHardware.baudrate;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
      }

      if (text.indexOf('P1 Smart Meter') >= 0) {
        const baudrate = this.selectedHardware.baudratep1;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
        Mode2 = this.selectedHardware.disablecrcp1 ? 0 : 1;
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '0';
        }
        Mode3 = ratelimitp1;
      }
      if (text.indexOf('Teleinfo EDF') >= 0) {
        const baudrate = this.selectedHardware.baudrateteleinfo;

        if (typeof baudrate === 'undefined') {
          this.notificationService.ShowNotify(this.translationService.t('No baud rate selected!'), 2500, true);
          return;
        }

        Mode1 = baudrate;
        Mode2 = this.selectedHardware.disablecrcp1 ? 0 : 1;
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '60';
        }
        Mode3 = ratelimitp1;
      }

      if (text.indexOf('S0 Meter') >= 0) {
        extra = this.devExtra;
      }

      if (text.indexOf('Denkovi') >= 0) {
        Mode1 = this.selectedHardware.modeldenkoviusbdevices;
      }

      if (text.indexOf('USBtin') >= 0) {
        // var Typecan = $("#hardwarecontent #divusbtin #combotypecanusbtin option:selected").val();
        const ActivateMultiblocV8 = this.selectedHardware.activateMultiblocV8 ? 1 : 0;
        const ActivateCanFree = this.selectedHardware.activateCanFree ? 1 : 0;
        const DebugActiv = this.selectedHardware.debugusbtin;
        // tslint:disable-next-line:no-bitwise
        Mode1 = (ActivateCanFree & 0x01);
        // tslint:disable-next-line:no-bitwise
        Mode1 <<= 1;
        // tslint:disable-next-line:no-bitwise
        Mode1 += (ActivateMultiblocV8 & 0x01);
        Mode2 = DebugActiv;
      }

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        extra, bEnabled, datatimeout, undefined, undefined, undefined, serialport, undefined)
        .subscribe(() => {
          if ((bEnabled) && (text.indexOf('RFXCOM') >= 0)) {
            this.notificationService.ShowNotify(this.translationService.t('Please wait. Updating ....!'), 2500);
            timer(3000).subscribe(() => {
              this.hideAndRefreshHardwareTable();
            });
          } else {
            this.RefreshHardwareTable();
          }
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('LAN') >= 0 &&
        text.indexOf('YouLess') === -1 &&
        text.indexOf('Denkovi') === -1 &&
        text.indexOf('Relay-Net') === -1 &&
        text.indexOf('Satel Integra') === -1 &&
        text.indexOf('eHouse') === -1 &&
        text.indexOf('ETH8020') === -1 &&
        text.indexOf('Daikin') === -1 &&
        text.indexOf('Sterbox') === -1 &&
        text.indexOf('Anna') === -1 &&
        text.indexOf('KMTronic') === -1 &&
        text.indexOf('MQTT') === -1 &&
        text.indexOf('Razberry') === -1 &&
        text.indexOf('MyHome OpenWebNet with LAN interface') === -1 &&
        text.indexOf('EnphaseAPI') === -1
      )
    ) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      let extra = '';
      if (text.indexOf('Evohome') >= 0) {
        extra = this.selectedHardware.controlleridevohometcp;
      }

      if (text.indexOf('S0 Meter') >= 0) {
        extra = this.devExtra;
      }

      if (text.indexOf('P1 Smart Meter') >= 0) {
        Mode2 = this.selectedHardware.disablecrcp1 ? 0 : 1;
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '0';
        }
        Mode3 = ratelimitp1;
      }
      if (text.indexOf('Teleinfo EDF') >= 0) {
        Mode2 = this.selectedHardware.disablecrcp1 ? 0 : 1;
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '60';
        }
        Mode3 = ratelimitp1;
      }

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        extra, bEnabled, datatimeout, undefined, undefined, address, port, undefined)
        .subscribe(() => {
          if ((bEnabled) && (text.indexOf('RFXCOM') >= 0)) {
            this.notificationService.ShowNotify(this.translationService.t('Please wait. Updating ....!'), 2500);
            timer(3000).subscribe(() => {
              this.hideAndRefreshHardwareTable();
            });
          } else {
            this.RefreshHardwareTable();
          }
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('LAN') >= 0 && ((text.indexOf('YouLess') >= 0) || (text.indexOf('Denkovi') >= 0))) ||
      (text.indexOf('Relay-Net') >= 0) || (text.indexOf('Satel Integra') >= 0) || (text.indexOf('eHouse') >= 0) ||
      (text.indexOf('Harmony') >= 0) || (text.indexOf('Xiaomi Gateway') >= 0) ||
      (text.indexOf('MyHome OpenWebNet with LAN interface') >= 0)
    ) {
      let address = this.selectedHardware.tcpaddress;
      if (text.indexOf('eHouse') >= 0) {
        if (address === '') {
          address = '192.168.0.200';
        }
      }
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      let port = this.selectedHardware.tcpport;
      if (text.indexOf('eHouse') >= 0) {
        if (port === '') {
          port = '9876';
        }
      }

      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }

      if (text.indexOf('Satel Integra') >= 0) {
        const pollinterval = String(this.selectedHardware.pollinterval);
        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }
        Mode1 = pollinterval;
      } else if (text.indexOf('eHouse') >= 0) {
        const pollinterval = String(this.selectedHardware.pollinterval);
        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }
        Mode1 = pollinterval;
        Mode2 = this.selectedHardware.ehouseautodiscovery ? 1 : 0;
        Mode3 = this.selectedHardware.ehouseaddalarmin ? 1 : 0;
        Mode4 = this.selectedHardware.ehouseprodiscovery ? 1 : 0;
        Mode5 = this.selectedHardware.ehouseopts;
        Mode6 = this.selectedHardware.ehouseopts2;
      }
      if (text.indexOf('Denkovi') >= 0) {
        const pollinterval = String(this.selectedHardware.pollinterval);
        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }
        Mode1 = pollinterval;
        if (text.indexOf('Modules with LAN (HTTP)') >= 0) {
          Mode2 = this.selectedHardware.modeldenkovidevices;
        } else if (text.indexOf('Modules with LAN (TCP)') >= 0) {
          Mode2 = this.selectedHardware.modeldenkovitcpdevices;
          Mode3 = this.selectedHardware.denkovislaveid;
          if (Mode2 === '1') {
            // const intRegex = /^\d+$/;
            if (isNaN(Mode3) || Number(Mode3) < 1 || Number(Mode3) > 247) {
              this.notificationService.ShowNotify(this.translationService.t('Invalid Slave ID! Enter value from 1 to 247!'), 2500, true);
              return;
            }
          } else {
            Mode3 = '0';
          }
        }
        Mode4 = '0';
        Mode5 = '0';
        Mode6 = '0';
      } else if (text.indexOf('Relay-Net') >= 0) {
        Mode1 = this.selectedHardware.relaynetpollinputs ? 1 : 0;
        Mode2 = this.selectedHardware.relaynetpollrelays ? 1 : 0;
        const pollinterval = String(this.selectedHardware.relaynetpollinterval);
        const inputcount = String(this.selectedHardware.relaynetinputcount);
        const relaycount = String(this.selectedHardware.relaynetrelaycount);

        if (pollinterval === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
          return;
        }

        if (inputcount === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter input count!'), 2500, true);
          return;
        }

        if (relaycount === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter relay count!'), 2500, true);
          return;
        }

        Mode3 = pollinterval;
        Mode4 = inputcount;
        Mode5 = relaycount;
      }
      const password = this.selectedHardware.password;
      if (text.indexOf('eHouse') >= 0) {
        if (password === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter ASCI password - 6 characters'), 2500, true);
        }
      }
      if (text.indexOf('MyHome OpenWebNet with LAN interface') >= 0) {
        if (password !== '') {
          if ((password.length < 5) || (password.length > 16)) {
            this.notificationService.ShowNotify(
              this.translationService.t('Please enter a password between 5 and 16 characters'), 2500, true);
            return;
          }

          // const intRegex2 = /^[a-zA-Z0-9]*$/;
          // if (!intRegex2.test(password)) {
          //   this.notificationService.ShowNotify(this.translationService.t('Please enter a numeric or alphanumeric (for HMAC) password'),
          //     2500, true);
          //   return;
          // }
        }

        const ratelimitp1 = $('#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1').val();
        if ((ratelimitp1 === '') || (isNaN(ratelimitp1))) {
          this.notificationService.ShowNotify(this.translationService.t('Please enter rate limit!'), 2500, true);
          return;
        }
        Mode1 = ratelimitp1;
      }
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, undefined, password, address, port, undefined)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (
      (text.indexOf('Domoticz') >= 0) ||
      (text.indexOf('Eco Devices') >= 0) ||
      (text.indexOf('ETH8020') >= 0) ||
      (text.indexOf('Daikin') >= 0) ||
      (text.indexOf('Sterbox') >= 0) ||
      (text.indexOf('Anna') >= 0) ||
      (text.indexOf('KMTronic') >= 0) ||
      (text.indexOf('MQTT') >= 0) ||
      (text.indexOf('Razberry') >= 0)
    ) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      let extra = '';
      Mode1 = '';
      Mode2 = '';
      Mode3 = '';
      if (text.indexOf('MySensors Gateway with MQTT') >= 0) {
        extra = this.selectedHardware.filename;
        Mode1 = this.selectedHardware.topicselect;
        Mode2 = this.selectedHardware.tlsversion;
        Mode3 = this.selectedHardware.preventloop;
        if (this.selectedHardware.filename.indexOf('#') >= 0) {
          this.notificationService.ShowNotify(this.translationService.t('CA Filename cannot contain a "#" symbol!'), 2500, true);
          return;
        }
        if (this.selectedHardware.mqtttopicin.indexOf('#') >= 0) {
          this.notificationService.ShowNotify(this.translationService.t('Publish Prefix cannot contain a "#" symbol!'), 2500, true);
          return;
        }
        if (this.selectedHardware.mqtttopicout.indexOf('#') >= 0) {
          this.notificationService.ShowNotify(this.translationService.t('Subscribe Prefix cannot contain a "#" symbol!'), 2500, true);
          return;
        }
        if ((Mode1 === 2) && ((this.selectedHardware.mqtttopicin === '') || (this.selectedHardware.mqtttopicout === ''))) {
          this.notificationService.ShowNotify(this.translationService.t('Please enter Topic Prefixes!'), 2500, true);
          return;
        }
        if (Mode1 === 2) {
          if ((this.selectedHardware.mqtttopicin === '') || (this.selectedHardware.mqtttopicout === '')) {
            this.notificationService.ShowNotify(this.translationService.t('Please enter Topic Prefixes!'), 2500, true);
            return;
          }
          extra += '#';
          extra += this.selectedHardware.mqtttopicin;
          extra += '#';
          extra += this.selectedHardware.mqtttopicout;
        }
      } else if ((text.indexOf('MQTT') >= 0)) {
        extra = this.selectedHardware.filename;
        Mode1 = this.selectedHardware.topicselect;
        Mode2 = this.selectedHardware.tlsversion;
        Mode3 = this.selectedHardware.preventloop;
      }
      if (text.indexOf('Eco Devices') >= 0) {
        Mode1 = this.selectedHardware.modelecodevices;
        let ratelimitp1 = String(this.selectedHardware.ratelimitp1);
        if (ratelimitp1 === '') {
          ratelimitp1 = '60';
        }
        Mode2 = ratelimitp1;
      }

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        extra, bEnabled, datatimeout, username, password, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Philips Hue') >= 0) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;
      if (username === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a username!'), 2500, true);
        return;
      }
      const pollinterval = String(this.selectedHardware.pollinterval);
      if (pollinterval === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter poll interval!'), 2500, true);
        return;
      }
      Mode1 = pollinterval;
      Mode2 = 0;
      if (this.selectedHardware.addgroups) {
        // tslint:disable-next-line:no-bitwise
        Mode2 |= 1;
      }
      if (this.selectedHardware.addscenes) {
        // tslint:disable-next-line:no-bitwise
        Mode2 |= 2;
      }
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, username, undefined, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if ((text.indexOf('HTTP/HTTPS') >= 0)) {
      const url = this.selectedHardware.url;
      if (url === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an url!'), 2500, true);
        return;
      }
      const script = this.selectedHardware.script;
      if (script === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a script!'), 2500, true);
        return;
      }
      const refresh = String(this.selectedHardware.refresh);
      if (refresh === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a refresh rate!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      const method = this.selectedHardware.method;
      if (typeof method === 'undefined') {
        this.notificationService.ShowNotify(this.translationService.t('No HTTP method selected!'), 2500, true);
        return;
      }
      const contenttype = this.selectedHardware.contenttype;
      const headers = this.selectedHardware.headers;
      const postdata = this.selectedHardware.postdata;
      let extra = btoa(script) + '|' + btoa(String(method)) + '|' + btoa(contenttype) + '|' + btoa(headers);
      if (method === 1) {
        extra = extra + '|' + btoa(postdata);
      }

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, '', '', '', '', '', '',
        extra, bEnabled, datatimeout, username, password, url, refresh)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if ((text.indexOf('Underground') >= 0) || (text.indexOf('DarkSky') >= 0) ||
      (text.indexOf('AccuWeather') >= 0) || (text.indexOf('Open Weather Map') >= 0)) {
      const apikey = this.selectedHardware.apikey;
      if (apikey === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an API Key!'), 2500, true);
        return;
      }
      const location = this.selectedHardware.location;
      if (location === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Location!'), 2500, true);
        return;
      }
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, apikey, location)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Buienradar') >= 0) {
      let timeframe = this.selectedHardware.timeframe;
      if (timeframe === 0) {
        timeframe = 30;
      }
      let threshold = this.selectedHardware.threshold;
      if (threshold === 0) {
        threshold = 25;
      }
      const location = this.selectedHardware.location;
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, timeframe.toString(), threshold.toString(),
        Mode3, Mode4, Mode5, Mode6, undefined, bEnabled, datatimeout, undefined, location)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('SolarEdge via Web') >= 0) {
      const apikey = this.selectedHardware.apikey;
      if (apikey === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an API Key!'), 2500, true);
        return;
      }

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, '', '', '', '', '', '',
        undefined, bEnabled, datatimeout, apikey)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Nest Th') >= 0 && text.indexOf('OAuth') >= 0) {
      const apikey = this.selectedHardware.apikey;
      const productid = this.selectedHardware.productid;
      const productsecret = this.selectedHardware.productsecret;
      const productpin = this.selectedHardware.productpin;

      if (apikey === '' && (productid === '' || productsecret === '' || productpin === '')) {
        this.notificationService.ShowNotify(
          this.translationService.t('Please enter an API Key or a combination of Product Id, Product Secret and PIN!'), 2500, true);
        return;
      }

      const extra = btoa(productid) + '|' + btoa(productsecret) + '|' + btoa(productpin);
      // console.log("Updating extra1: " + extra);

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, '', '', '', '', '', '',
        extra, bEnabled, datatimeout, apikey)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('SBFSpot') >= 0) {
      const configlocation = this.selectedHardware.location;
      if (configlocation === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Location!'), 2500, true);
        return;
      }
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, configlocation)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Toon') >= 0) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      const agreement = String(this.selectedHardware.agreement);
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, agreement, '', '', '', '', '',
        undefined, bEnabled, datatimeout, username, password)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Tesla') >= 0) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      const vinnr = this.selectedHardware.teslavinnr;
      let activeinterval = this.selectedHardware.teslaactiveinterval;
      if (activeinterval < 1) {
        activeinterval = 1;
      }
      let defaultinterval = this.selectedHardware.tesladefaultinterval;
      if (defaultinterval < 1) {
        defaultinterval = 20;
      }
      const allowwakeup = this.selectedHardware.teslaallowwakeup;
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, defaultinterval.toString(), activeinterval.toString(), allowwakeup.toString(), '', '', '',
        vinnr, bEnabled, datatimeout, username, password).subscribe(() => {
        this.RefreshHardwareTable();
      }, error => {
        this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
      });
    } else if (
      (text.indexOf('ICY') >= 0) ||
      (text.indexOf('Atag') >= 0) ||
      (text.indexOf('Nest Th') >= 0 && text.indexOf('OAuth') === -1) ||
      (text.indexOf('PVOutput') >= 0) ||
      (text.indexOf('Netatmo') >= 0) ||
      (text.indexOf('Thermosmart') >= 0) ||
      (text.indexOf('Tado') >= 0)
    ) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, username, password)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Winddelen') >= 0) {
      const mill_id = this.selectedHardware.millselect;
      const mill_name = this.millselectOptions.find(_ => _.value === mill_id).label;
      const nrofwinddelen = this.selectedHardware.nrofwinddelen;
      const intRegex = /^\d+$/;
      if (!intRegex.test(nrofwinddelen)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Number!'), 2500, true);
        return;
      }
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, String(mill_id), Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, undefined, undefined, mill_name, nrofwinddelen)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Honeywell') >= 0) {
      const apiKey = this.selectedHardware.apikey;
      const apiSecret = this.selectedHardware.hwApiSecret;
      const accessToken = this.selectedHardware.hwAccessToken;
      const refreshToken = this.selectedHardware.hwRefreshToken;
      const extra = btoa(apiKey) + '|' + btoa(apiSecret);

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        extra, bEnabled, datatimeout, accessToken, refreshToken)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Logitech Media Server') >= 0) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, username, password, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('HEOS by DENON') >= 0) {
      const address = this.selectedHardware.tcpaddress;
      if (address === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Address!'), 2500, true);
        return;
      }
      const port = this.selectedHardware.tcpport;
      if (port === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Port!'), 2500, true);
        return;
      }
      const intRegex = /^\d+$/;
      if (!intRegex.test(port)) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Valid Port!'), 2500, true);
        return;
      }
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, username, password, address, port)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Goodwe solar inverter via Web') >= 0) {
      const username = this.selectedHardware.username;
      if (username === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter your Goodwe username!'), 2500, true);
        return;
      }
      Mode1 = this.selectedHardware.serverselect;

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6,
        undefined, bEnabled, datatimeout, username)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Evohome via Web') >= 0) {
      const username = this.selectedHardware.username;
      const password = this.selectedHardware.password;

      let Pollseconds = this.selectedHardware.updatefrequencyevohomeweb;
      if (Pollseconds < 10) {
        Pollseconds = 60;
      }

      let UseFlags = 0;
      if (this.selectedHardware.showlocationevohomeweb) {
        this.selectedHardware.disableautoevohomeweb = true;
        // tslint:disable-next-line:no-bitwise
        UseFlags = UseFlags | 4;
      }

      if (!this.selectedHardware.disableautoevohomeweb) { // reverted value - default 0 is true
        // tslint:disable-next-line:no-bitwise
        UseFlags = UseFlags | 1;
      }

      if (this.selectedHardware.showscheduleevohomeweb) {
        // tslint:disable-next-line:no-bitwise
        UseFlags = UseFlags | 2;
      }

      const Precision = this.selectedHardware.evoprecision;
      UseFlags += Precision;

      let evo_installation = this.selectedHardware.comboevolocation * 4096;
      evo_installation = evo_installation + this.selectedHardware.comboevogateway * 256;
      evo_installation = evo_installation + this.selectedHardware.comboevotcs * 16;

      this.hardwareService.updateHardware(Number(hardwaretype), idx, name,
        String(Pollseconds), UseFlags.toString(), evo_installation.toString(), '', '', '',
        undefined, bEnabled, datatimeout, username, password)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    } else if (text.indexOf('Rtl433 RTL-SDR receiver') >= 0) {
      this.hardwareService.updateHardware(Number(hardwaretype), idx, name, '', '', '', '', '', '',
        this.selectedHardware.rtl433cmdline, bEnabled, datatimeout)
        .subscribe(() => {
          this.RefreshHardwareTable();
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem updating hardware!'), 2500, true);
        });
    }
  }

  get typeLabel(): string {
    const _text = this.hardwareTypes.find(_ => _.idx === this.selectedHardware.type);
    let text = '';
    if (_text === undefined) {
      if (this.selectedHardware.type === 1000) {
        text = 'I2C sensors';
      } else {
        const __text = this.hardwareTypes.find(_ => _.key === this.selectedHardware.type);
        if (__text !== undefined) {
          text = __text.name;
        }
      }
    } else {
      text = _text.name;
    }
    return text;
  }

  hideAndRefreshHardwareTable() {
    this.notificationService.HideNotify();
    this.RefreshHardwareTable();
  }

  private CreateDummySensors(idx: string, name: string) {
    const dialog = this.dialogService.addDialog(CreateDummySensorDialogComponent, {idx: idx, name: name}).instance;
    dialog.init();
    dialog.open();
  }

  private AddYeeLight(idx: string, name: string) {
    const dialog = this.dialogService.addDialog(AddYeeLightDialogComponent, {idx: idx, name: name}).instance;
    dialog.init();
    dialog.open();
  }

  private ReloadPiFace(idx: string) {
    this.hardwareService.ReloadPiFace(idx).subscribe(() => {
      this.notificationService.ShowNotify(this.translationService.t('PiFace config reloaded!'), 2500);
    });
  }

  private CreateRFLinkDevices(idx: string) {
    const dialog = this.dialogService.addDialog(CreateRfLinkDeviceDialogComponent, {idx: idx}).instance;
    dialog.init();
    dialog.open();
  }

  private CreateEvohomeSensors(idx: string) {
    const dialog = this.dialogService.addDialog(CreateEvohomeSensorsDialogComponent, {idx: idx}).instance;
    dialog.init();
    dialog.open();
  }

  private BindEvohome(idx: string, devtype: string) {
    if (devtype === 'Relay') {
      this.notificationService.ShowNotify(this.translationService.t('Hold bind button on relay...'), 2500);
    } else if (devtype === 'AllSensors') {
      this.notificationService.ShowNotify(this.translationService.t('Creating additional sensors...'));
    } else if (devtype === 'ZoneSensor') {
      this.notificationService.ShowNotify(this.translationService.t('Binding Domoticz zone temperature sensor to controller...'), 2500);
    } else {
      this.notificationService.ShowNotify(
        this.translationService.t('Binding Domoticz outdoor temperature device to evohome controller...'), 2500);
    }

    let bNewDevice = false;
    let bIsUsed = false;
    let Name = '';

    timer(600).pipe(
      mergeMap(() => {
        return this.hardwareService.bindEvoHome(idx, devtype);
      }),
      tap((data: EvoHomeResponse) => {
        if (typeof data.status !== 'undefined') {
          bIsUsed = data.Used;
          if (data.status === 'OK') {
            bNewDevice = true;
          } else {
            Name = data.Name;
          }
        }
      }),
      tap(() => {
        this.notificationService.HideNotify();
      }),
      delay(200)
    ).subscribe(() => {
      if ((bNewDevice === true) && (bIsUsed === false)) {
        if (devtype === 'Relay') {
          this.notificationService.ShowNotify(this.translationService.t('Relay bound, and can be found in the devices tab!'), 2500);
        } else if (devtype === 'AllSensors') {
          this.notificationService.ShowNotify(this.translationService.t('New sensors will appear in the devices tab after 10min'), 2500);
        } else if (devtype === 'ZoneSensor') {
          this.notificationService.ShowNotify(this.translationService.t('Sensor bound, and can be found in the devices tab!'), 2500);
        } else {
          this.notificationService.ShowNotify(
            this.translationService.t('Domoticz outdoor temperature device has been bound to evohome controller'), 2500);
        }
      } else {
        if (bIsUsed === true) {
          this.notificationService.ShowNotify(this.translationService.t('Already used by') + ':<br>"' + Name + '"', 3500, true);
        } else {
          this.notificationService.ShowNotify(this.translationService.t('Timeout...<br>Please try again!'), 2500, true);
        }
      }
    });
  }

  private AddArilux(idx: string) {
    const dialog = this.dialogService.addDialog(AddAriluxDialogComponent, {idx: idx}).instance;
    dialog.init();
    dialog.open();
  }

  // AngularJS code not used

  // credits: http://www.netlobo.com/url_query_string_javascript.html
  // function gup(url, name) {
  //   name = name.replace(/[[]/, "\[").replace(/[]]/, "\]");
  //   var regexS = "[\?&]" + name + "=([^&#]*)";
  //   var regex = new RegExp(regexS);
  //   var results = regex.exec(url);
  //   if (results == null)
  //     return "";
  //   else
  //     return results[1];
  // }

  // OnAuthenticateToon = function () {
  //   var pwidth = 800;
  //   var pheight = 600;

  //   var dualScreenLeft = window.screenLeft != undefined ? window.screenLeft : screen.left;
  //   var dualScreenTop = window.screenTop != undefined ? window.screenTop : screen.top;

  //   var width = window.innerWidth ? window.innerWidth : document.documentElement.clientWidth ? document.documentElement.clientWidth : screen.width;
  //   var height = window.innerHeight ? window.innerHeight : document.documentElement.clientHeight ? document.documentElement.clientHeight : screen.height;

  //   var left = ((width / 2) - (pwidth / 2)) + dualScreenLeft;
  //   var top = ((height / 2) - (pheight / 2)) + dualScreenTop;

  //   var REDIRECT = 'http://127.0.0.1/domoticiz_toon';
  //   var CLIENT_ID = '7gQMPclYzm8haCHAgdvjq1yILLwa';
  //   var _url = 'https://api.toonapi.com/authorize?response_type=code&redirect_uri=' + REDIRECT + '&client_id=' + CLIENT_ID;
  //   //_url = "http://127.0.0.1:8081/11";
  //   var win = window.open(_url, "windowtoonaith", 'scrollbars=yes, width=' + pwidth + ', height=' + pheight + ', left=' + left + ', top=' + top);
  //   if (window.focus) {
  //     win.focus();
  //   }
  //   var pollTimer = window.setInterval(function () {
  //     if (win.closed !== false) { // !== is required for compatibility with Opera
  //       window.clearInterval(pollTimer);
  //     }
  //     else if (win.document.URL.indexOf(REDIRECT) != -1) {
  //       window.clearInterval(pollTimer);
  //       console.log(win.document.URL);
  //       var url = win.document.URL;
  //       var code = gup(url, 'code');
  //       win.close();
  //       validateToonToken(code);
  //     }
  //   }, 200);
  // }

}

interface HardwareRow {
  DT_RowId: number;
  Username: string;
  Password: string;
  Extra: string;
  Enabled: string;
  Name: string;
  Mode1: number;
  Mode2: number;
  Mode3: number;
  Mode4: number;
  Mode5: number;
  Mode6: number;
  Type: string;
  IntPort: number;
  Address: string;
  Port: string;
  DataTimeout: number;
  0: number;
  1: string;
  2: string;
  3: string;
  4: string;
  5: string;
  6: string;
}

interface SelectedHardware {
  comboevotcs?: number;
  comboevogateway?: number;
  comboevolocation?: number;
  evoprecision?: number;
  showlocationevohomeweb?: boolean;
  showscheduleevohomeweb?: boolean;
  disableautoevohomeweb?: boolean;
  updatefrequencyevohomeweb?: number;
  rtl433cmdline?: string;
  topicselect?: number;
  filename?: string;
  serverselect?: number;
  hwApiSecret?: string;
  hwApiKey?: string;
  hwAccessToken?: string;
  hwRefreshToken?: string;
  nrofwinddelen?: string;
  millselect?: number;
  addscenes?: number;
  addgroups?: number;
  username?: string;
  agreement?: number;
  productpin?: string;
  productsecret?: string;
  productid?: string;
  refresh?: number;
  postdata?: string;
  headers?: string;
  contenttype?: string;
  method?: number;
  script?: string;
  url?: string;
  location?: string;
  apikey?: string;
  denkovislaveid?: number;
  modeldenkovitcpdevices?: number;
  modeldenkovidevices?: number;
  relaynetrelaycount?: number;
  relaynetinputcount?: number;
  relaynetpollinterval?: number;
  relaynetpollrelays?: boolean;
  relaynetpollinputs?: boolean;
  ehouseopts2?: number;
  ehouseopts?: number;
  ehouseprodiscovery?: boolean;
  ehouseaddalarmin?: boolean;
  ehouseautodiscovery?: boolean;
  pollinterval?: number;
  password?: string;
  controlleridevohometcp?: string;
  modelecodevices?: number;
  tcpport?: string;
  tcpaddress?: string;
  modeldenkoviusbdevices?: number;
  debugusbtin?: number;
  activateCanFree?: boolean;
  activateMultiblocV8?: boolean;
  baudrateteleinfo?: number;
  ratelimitp1?: number;
  disablecrcp1?: boolean;
  baudratep1?: number;
  baudrate?: number;
  controllerid?: string;
  baudrateevohome?: number;
  serialport?: number;
  sysfsdebounce?: number;
  sysfsautoconfigure?: boolean;
  gpiopollinterval?: number;
  gpioperiod?: number;
  gpiodebounce?: number;
  i2cinvert?: boolean;
  i2caddress?: string;
  i2cpath?: string;
  i2clocal?: number;
  OneWireSwitchPollPeriod?: number;
  OneWireSensorPollPeriod?: number;
  owfspath?: string;
  timeout?: number;
  type?: number | string;
  enabled?: boolean;
  name?: string;
  mqtttopicout?: string;
  mqtttopicin?: string;
  timeframe?: number;
  threshold?: number;
  tlsversion?: number;
  teslavinnr?: string;
  teslaactiveinterval?: number;
  tesladefaultinterval?: number;
  teslaallowwakeup?: number;
  preventloop?: number;
}
