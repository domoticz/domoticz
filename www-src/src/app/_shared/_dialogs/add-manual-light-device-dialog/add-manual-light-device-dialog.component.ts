import {Component, OnInit, AfterViewInit, Inject} from '@angular/core';
import {DialogComponent} from 'src/app/_shared/_dialogs/dialog.component';
import {DialogService, DIALOG_DATA} from 'src/app/_shared/_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ReplaceDeviceService} from 'src/app/_shared/_services/replace-device.service';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {NotificationService} from '../../_services/notification.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {Hardware} from 'src/app/_shared/_models/hardware';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-add-manual-light-device-dialog',
  templateUrl: './add-manual-light-device-dialog.component.html',
  styleUrls: ['./add-manual-light-device-dialog.component.css']
})
export class AddManualLightDeviceDialogComponent extends DialogComponent implements OnInit, AfterViewInit {

  // FIXME replace those callbacks with a more Angular way to do so
  addCallback: () => any;

  hexaValues: Array<{ val: string, text: string }> = [];
  combohouseCodes: Array<{ val: string, text: string }> = [];
  groupcodeValues: Array<{ val: string, text: string }> = [];
  openwebnetcmd3Values: Array<{ val: string, text: string }> = [];
  lighting1housecodes: Array<{ val: string, text: string }> = [];
  homeconfortUnitCodes: Array<string> = [];
  lighting2unitcodes: Array<string> = [];
  lighting3unitcodes: Array<string> = [];
  lightingenoceanids: Array<string> = [];
  lightingenoceanunitcodes: Array<string> = [];
  openwebnetcmd1Values: Array<string> = [];
  openwebnetcmd2Values: Array<string> = [];
  openwebnetAUXcmd1Values: Array<string> = [];
  openwebnetZigbeecmd2Values: Array<string> = [];
  openwebnetDryContactcmd1Values: Array<string> = [];
  openwebnetIRdeteccmd1Values: Array<string> = [];
  openwebnetIRdeteccmd2Values: Array<string> = [];
  openwebnetCustomcmd1Values: Array<string> = [];
  openwebnetCustomcmd2Values: Array<string> = [];
  lighting1unitcodes: Array<string> = [];
  ComboHardware: Array<Hardware> = [];
  ComboGpio: Array<Hardware> = [];
  ComboSysfsGpio: Array<Hardware> = [];

  switchtype = 0;
  lighttype = 0;
  blindscombocmd1 = '';
  blindscombocmd2 = '';
  blindscombocmd3 = '';
  blindscombocmd4 = '';
  hardware: number;
  devicename = '';
  groupcode = '';
  unitcode = '';
  cmd1 = '';
  cmd2 = '';
  enoceanid = '';
  enoceanunitcode = '';
  gpio: number;
  lighting2cmd1 = '0';
  lighting2cmd2: string;
  lighting2cmd3: string;
  lighting2cmd4: string;
  sysfsgpio: number;
  lighting1housecode: string;
  lighting1unitcode: string;
  he105unitcode: string;
  blindsunitcode: string;
  homeconfortcmd1: string;
  homeconfortcmd2: string;
  homeconfortcmd3: string;
  homeconfortunitcode: string;
  homeconforthousecode: string;
  fancmd1: string;
  fancmd2: string;
  fancmd3: string;
  openwebnetcmd1: string;
  openwebnetcmd2: string;
  openwebnetcmd3: string;
  openwebnetAUXcmd1: string;
  zigbeecmd1: string;
  zigbeecmd2: string;
  openwebnetdrycontactcmd1: string;
  openwebnetIRdeteccmd1: string;
  openwebnetIRdeteccmd2: string;
  openwebnetcustomcmd1: string;
  openwebnetcustomcmd2: string;
  openwebnetcustominputcmd1: string;
  lighting2unitcode: string;
  how = 'New';
  subdevice: string;

  displayHe105Params = false;
  displayBlindsparams = false;
  displayLightingparams_enocean = false;
  displayLightingparams_gpio = false;
  displayLightingparams_sysfsgpio = false;
  displayhomeconfortparams = false;
  displayfanparams = false;
  displayopenwebnetparamsBus = false;
  displayopenwebnetparamsAUX = false;
  displayopenwebnetparamsZigbee = false;
  displayopenwebnetparamsDryContact = false;
  displayopenwebnetparamsIRdetec = false;
  displayopenwebnetparamsCustom = false;
  displaylighting1params = false;
  displaylighting2params = false;
  displaylighting3params = false;
  displaylighting2paramsUnitCode = false;
  displayblindsparamscombocmd1 = false;
  displayblindsparamscombocmd4 = false;
  displayblindparamsUnitCode = false;
  displaylighting2cmd1 = false;
  displaylighting2cmd2 = false;

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private replaceDeviceService: ReplaceDeviceService,
    private deviceService: DeviceService,
    private notificationService: NotificationService,
    @Inject(DIALOG_DATA) data: any) {

    super(dialogService);

    this.addCallback = data.addCallback;
  }

  ngOnInit() {
    for (let ii = 0; ii < 256; ii++) {
      this.hexaValues.push({val: ii.toString(), text: Utils.strPad(ii.toString(16).toUpperCase(), 2)});
    }

    for (let ii = 0; ii < 4; ii++) {
      this.combohouseCodes.push({val: (65 + ii).toString(), text: String.fromCharCode(65 + ii)});
      this.homeconfortUnitCodes.push((ii + 1).toString());
    }

    this.lighting2unitcodes = this.hexaValues.slice(1, 17).map(_ => _.val);

    this.RefreshHardwareComboArray();
    this.RefreshGpioComboArray();
    this.RefreshSysfsGpioComboArray();
  }

  ngAfterViewInit() {
  }

  protected getDialogId(): string {
    return 'dialog-addmanuallightdevice';
  }

  protected getDialogTitle(): string {
    return this.translationService.t('Add Manual Light/Switch Device');
  }

  protected getDialogOptions(): any {
    return {
      width: 440,
      height: 480
    };
  }

  protected onOpen() {
    this.UpdateAddManualDialog();
  }

  public onSwitchTypeChange() {
    let subtype = -1;
    if (this.switchtype === 1) {
      subtype = 10;	// Doorbell -> COCO GDR2
    } else if (this.switchtype === 18) {
      subtype = 303;	// Selector -> Selector Switch
    }
    if (subtype !== -1) {
      this.lighttype = subtype;
    }
    this.UpdateAddManualDialog();
  }

  public onLightTypeChange() {
    let switchtype = -1;
    if (this.lighttype === 303) {
      switchtype = 18;	// Selector -> Selector Switch
    }
    if (switchtype !== -1) {
      this.switchtype = switchtype;
    }
    this.UpdateAddManualDialog();
  }

  protected getDialogButtons(): any {

    const dialog_addmanuallightdevice_buttons = {};

    dialog_addmanuallightdevice_buttons[this.translationService.t('Add Device')] = () => {
      const mParams = this.GetManualLightSettings(false);
      if (mParams === {}) {
        return;
      }

      this.deviceService.addSwitch(mParams).subscribe((data) => {
        if (typeof data.status !== 'undefined') {
          if (data.status === 'OK') {
            this.close();
            // ShowLights();
            this.addCallback();
          } else {
            this.notificationService.ShowNotify(data.message, 3000, true);
          }
        }
      });
    };

    dialog_addmanuallightdevice_buttons[this.translationService.t('Cancel')] = () => {
      this.close();
    };

    return dialog_addmanuallightdevice_buttons;
  }

  TestManualLight() {
    const mParams = this.GetManualLightSettings(true);
    if (mParams === {}) {
      return;
    }
    this.deviceService.testSwitch(mParams).subscribe((data) => {
      if (typeof data.status !== 'undefined') {
        if (data.status !== 'OK') {
          const msg = this.translationService.t('There was an error!<br>Wrong switch parameters?') +
            ((typeof data.message !== 'undefined') ? '<br>' + data.message : '');
          this.notificationService.ShowNotify(msg, 2500, true);
        } else {
          this.notificationService.ShowNotify(this.translationService.t('\'On\' command send!'), 2500);
        }
      }
    });
  }

  private RefreshHardwareComboArray() {
    this.deviceService.getManualHardware().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.ComboHardware = data.result;
        this.hardware = this.ComboHardware[0].idx as number;
      }
    });
  }

  private RefreshGpioComboArray() {
    this.deviceService.getGpio().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.ComboGpio = data.result;
        this.gpio = this.ComboGpio[0].idx as number;
      }
    });
  }

  private RefreshSysfsGpioComboArray() {
    this.deviceService.getSysfsGpio().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.ComboSysfsGpio = data.result;
        this.sysfsgpio = this.ComboSysfsGpio[0].idx as number;
      }
    });
  }

  private UpdateAddManualDialog() {
    const lighttype = this.lighttype;
    const bIsARCType = ((lighttype < 20) || (lighttype === 101));
    let bIsType5 = 0;

    let tothousecodes = 1;
    let totunits = 1;
    let totrooms: number;
    let totpointofloads: number;
    let totbus: number;
    if ((lighttype === 0) || (lighttype === 1) || (lighttype === 3) || (lighttype === 101)) {
      tothousecodes = 16;
      totunits = 16;
    } else if ((lighttype === 2) || (lighttype === 5)) {
      tothousecodes = 16;
      totunits = 64;
    } else if (lighttype === 4) {
      tothousecodes = 3;
      totunits = 4;
    } else if (lighttype === 6) {
      tothousecodes = 4;
      totunits = 4;
    } else if (lighttype === 68) {
      // Do nothing. GPIO uses a custom form
    } else if (lighttype === 69) {
      // Do nothing. SysfsGpio uses a custom form
    } else if (lighttype === 7) {
      tothousecodes = 16;
      totunits = 8;
    } else if (lighttype === 8) {
      tothousecodes = 16;
      totunits = 4;
    } else if (lighttype === 9) {
      tothousecodes = 16;
      totunits = 10;
    } else if (lighttype === 10) {
      tothousecodes = 4;
      totunits = 4;
    } else if (lighttype === 11) {
      tothousecodes = 16;
      totunits = 64;
    } else if (lighttype === 106) {
      // Blyss
      tothousecodes = 16;
      totunits = 5;
    } else if (lighttype === 70) {
      // EnOcean
      tothousecodes = 128;
      totunits = 2;
    } else if (lighttype === 50) {
      // LightwaveRF
      bIsType5 = 1;
      totunits = 16;
    } else if (lighttype === 65) {
      // IT (Intertek,FA500,PROmax...)
      bIsType5 = 1;
      totunits = 4;
    } else if (lighttype === 55) {
      // Livolo
      bIsType5 = 1;
      totunits = 128;
    } else if (lighttype === 57) {
      // Aoke
      bIsType5 = 1;
    } else if (lighttype === 100) {
      // Chime/ByronSX
      bIsType5 = 1;
      totunits = 4;
    } else if ((lighttype === 102) || (lighttype === 107)) {
      // RFY/RFY2
      bIsType5 = 1;
      totunits = 16;
    } else if (lighttype === 103) {
      // Meiantech
      bIsType5 = 1;
      totunits = 0;
    } else if (lighttype === 105) {
      // ASA
      bIsType5 = 1;
      totunits = 16;
    } else if ((lighttype >= 200) && (lighttype < 300)) {
      // Blinds
    } else if (lighttype === 302) {
      // Home Confort
      tothousecodes = 4;
      totunits = 4;
    } else if ((lighttype === 400) || (lighttype === 401)) {
      // Openwebnet Bus Blinds/Lights
      totrooms = 11; // area, from 0 to 9 if physical configuration, 0 to 10 if virtual configuration
      totpointofloads = 16; // point of load, from 0 to 9 if physical configuration, 1 to 15 if virtual configuration
      totbus = 10; // maximum 10 local buses
    } else if (lighttype === 402) {
      // Openwebnet Bus Auxiliary
      totrooms = 10;
    } else if ((lighttype === 403) || (lighttype === 404)) {
      // Openwebnet Zigbee Blinds/Lights
      totunits = 3; // unit number is the button number on the switch (e.g. light1/light2 on a light switch)
    } else if (lighttype === 405) {
      // Openwebnet Bus Dry Contact
      totrooms = 200;
    } else if (lighttype === 406) {
      // Openwebnet Bus IR Detection
      totrooms = 10;
      totpointofloads = 10;
    } else if (lighttype === 407) {
      // Openwebnet Bus Custom
      totrooms = 200;
      totbus = 10; // maximum 10 local buses
    }

    this.displayHe105Params = false;
    this.displayBlindsparams = false;
    this.displayLightingparams_enocean = false;
    this.displayLightingparams_gpio = false;
    this.displayLightingparams_sysfsgpio = false;
    this.displayhomeconfortparams = false;
    this.displayfanparams = false;
    this.displayopenwebnetparamsBus = false;
    this.displayopenwebnetparamsAUX = false;
    this.displayopenwebnetparamsZigbee = false;
    this.displayopenwebnetparamsDryContact = false;
    this.displayopenwebnetparamsIRdetec = false;
    this.displayopenwebnetparamsCustom = false;

    if (lighttype === 104) {
      // HE105
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayHe105Params = true;
    } else if (lighttype === 303) {
      this.displaylighting1params = false;
      this.displaylighting2params = true;
      this.displaylighting3params = false;
    } else if (lighttype === 106) {
      // Blyss
      this.groupcodeValues = [];
      this.lighting3unitcodes = [];
      for (let ii = 0; ii < tothousecodes; ii++) {
        this.groupcodeValues.push({val: (41 + ii).toString(), text: String.fromCharCode(65 + ii)});
      }
      for (let ii = 1; ii < totunits + 1; ii++) {
        this.lighting3unitcodes.push(ii.toString());
      }
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = true;
    } else if (lighttype === 70) {
      // EnOcean
      this.lightingenoceanids = [];
      this.lightingenoceanunitcodes = [];
      for (let ii = 1; ii < tothousecodes + 1; ii++) {
        this.lightingenoceanids.push(ii.toString());
      }
      for (let ii = 1; ii < totunits + 1; ii++) {
        this.lightingenoceanunitcodes.push(ii.toString());
      }
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayLightingparams_enocean = true;
    } else if (lighttype === 68) {
      // GPIO
      this.lightingenoceanids = [];
      this.lightingenoceanunitcodes = [];
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayLightingparams_gpio = true;
    } else if (lighttype === 69) {
      // SysfsGpio
      this.lightingenoceanids = [];
      this.lightingenoceanunitcodes = [];
      this.displaylighting1params = false;
      this.displaylighting2params = true;
      this.displaylighting3params = false;
      this.displaylighting2paramsUnitCode = false;
      this.displayLightingparams_sysfsgpio = true;
    } else if ((lighttype >= 200) && (lighttype < 300)) {
      // Blinds
      this.displayBlindsparams = true;
      const bShow1 = (lighttype === 205) || (lighttype === 206) || (lighttype === 207) || (lighttype === 210) ||
        (lighttype === 211) || (lighttype === 250) || (lighttype === 226);
      const bShow4 = (lighttype === 206) || (lighttype === 207) || (lighttype === 209);
      const bShowUnit = (lighttype === 206) || (lighttype === 207) || (lighttype === 208) || (lighttype === 209) ||
        (lighttype === 212) || (lighttype === 213) || (lighttype === 215) || (lighttype === 250) || (lighttype === 226);
      if (bShow1) {
        this.displayblindsparamscombocmd1 = true;
      } else {
        this.displayblindsparamscombocmd1 = false;
        this.blindscombocmd1 = '0';
      }
      if (bShow4) {
        this.displayblindsparamscombocmd4 = true;
      } else {
        this.displayblindsparamscombocmd4 = false;
        this.blindscombocmd4 = '0';
      }
      if (bShowUnit) {
        this.displayblindparamsUnitCode = true;
      } else {
        this.displayblindparamsUnitCode = false;
      }

      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
    } else if (lighttype === 302) {
      // Home Confort
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayhomeconfortparams = true;
    } else if (lighttype === 304) {
      // Fan (Itho)
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayfanparams = true;
    } else if ((lighttype === 305) || (lighttype === 306) || (lighttype === 307)) {
      // Fan (Lucci Air/Westinghouse)
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayfanparams = true;
    } else if ((lighttype === 400) || (lighttype === 401)) {
      // Openwebnet Bus Blinds/Light
      this.openwebnetcmd1Values = [];
      for (let ii = 1; ii < totrooms; ii++) {
        this.openwebnetcmd1Values.push(ii.toString());
      }
      this.openwebnetcmd2Values = [];
      for (let ii = 1; ii < totpointofloads; ii++) {
        this.openwebnetcmd2Values.push(ii.toString());
      }
      this.openwebnetcmd3Values = [];
      this.openwebnetcmd3Values.push({val: '0', text: 'local bus'});
      for (let ii = 1; ii < totbus; ii++) {
        this.openwebnetcmd3Values.push({val: ii.toString(), text: ii.toString()});
      }

      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayopenwebnetparamsBus = true;
    } else if (lighttype === 402) {
      // Openwebnet Bus Auxiliary
      this.openwebnetAUXcmd1Values = [];
      for (let ii = 1; ii < totrooms; ii++) {
        this.openwebnetAUXcmd1Values.push(ii.toString());
      }
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayopenwebnetparamsAUX = true;
    } else if ((lighttype === 403) || (lighttype === 404)) {
      // Openwebnet Zigbee Blinds/Light
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayopenwebnetparamsBus = false;
      this.displayopenwebnetparamsAUX = false;
      this.displayopenwebnetparamsZigbee = true;
      this.openwebnetZigbeecmd2Values = [];
      for (let ii = 1; ii < totunits + 1; ii++) {
        this.openwebnetZigbeecmd2Values.push(ii.toString());
      }
    } else if (lighttype === 405) {
      // Openwebnet Dry Contact
      this.openwebnetDryContactcmd1Values = [];
      for (let ii = 1; ii < totrooms; ii++) {
        this.openwebnetDryContactcmd1Values.push(ii.toString());
      }

      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayopenwebnetparamsDryContact = true;
    } else if (lighttype === 406) {
      // Openwebnet IR Detection
      this.openwebnetIRdeteccmd1Values = [];
      for (let ii = 1; ii < totrooms; ii++) {
        this.openwebnetIRdeteccmd1Values.push(ii.toString());
      }
      this.openwebnetIRdeteccmd2Values = [];
      for (let ii = 1; ii < totpointofloads; ii++) {
        this.openwebnetIRdeteccmd2Values.push(ii.toString());
      }
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayopenwebnetparamsIRdetec = true;
    } else if (lighttype === 407) {
      // Openwebnet Bus Custom
      this.openwebnetCustomcmd1Values = [];
      for (let ii = 1; ii < totrooms + 1; ii++) {
        this.openwebnetCustomcmd1Values.push(ii.toString());
      }
      this.openwebnetCustomcmd2Values = [];
      for (let ii = 1; ii < totbus; ii++) {
        this.openwebnetCustomcmd2Values.push(ii.toString());
      }
      this.displaylighting1params = false;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
      this.displayopenwebnetparamsCustom = true;

    } else if (bIsARCType === true) {
      this.lighting1housecodes = [];
      this.lighting1unitcodes = [];
      for (let ii = 0; ii < tothousecodes; ii++) {
        this.lighting1housecodes.push({val: (65 + ii).toString(), text: String.fromCharCode(65 + ii)});
      }
      for (let ii = 1; ii < totunits + 1; ii++) {
        this.lighting1unitcodes.push(ii.toString());
      }
      this.displaylighting1params = true;
      this.displaylighting2params = false;
      this.displaylighting3params = false;
    } else {
      if (lighttype === 103) {
        this.displaylighting2paramsUnitCode = false;
      } else {
        this.displaylighting2paramsUnitCode = true;
        if (lighttype === 55) {
          this.lighting2unitcodes = [];
          for (let ii = 1; ii < totunits + 1; ii++) {
            this.lighting2unitcodes.push(ii.toString());
          }
        }
      }
      this.displaylighting2cmd2 = true;
      if (bIsType5 === 0) {
        this.displaylighting2cmd1 = true;
      } else {
        this.displaylighting2cmd1 = false;
        if ((lighttype === 55) || (lighttype === 57) || (lighttype === 65) || (lighttype === 100)) {
          this.displaylighting2cmd2 = false;
          this.lighting2cmd2 = '0';
          if ((lighttype !== 55) && (lighttype !== 65) && (lighttype !== 100)) {
            this.displaylighting2paramsUnitCode = false;
          }
        }
      }
      this.displaylighting1params = false;
      this.displaylighting2params = true;
      this.displaylighting3params = false;
    }
  }

  private GetManualLightSettings(isTest: boolean): any {
    const mParams: any = {};
    const hwdID = this.hardware;
    if (typeof hwdID === 'undefined') {
      this.notificationService.ShowNotify(this.translationService.t('No Hardware Device selected!'), 2500, true);
      return {};
    }
    mParams.hwdid = hwdID;

    const name = this.devicename;
    if ((name === '') || (!Utils.checkLength(name, 2, 100))) {
      if (!isTest) {
        this.notificationService.ShowNotify(this.translationService.t('Invalid Name!'), 2500, true);
        return {};
      }
    }
    mParams.name = name;

    const description = ''; // FIXME doesnt exist anymore
    mParams.description = description;

    mParams.switchtype = this.switchtype;
    const lighttype = this.lighttype;
    mParams.lighttype = lighttype;
    if (lighttype === 106) {
      // Blyss
      mParams.groupcode = this.groupcode;
      mParams.unitcode = this.unitcode;
      const ID = this.cmd1 + this.cmd2;
      mParams.id = ID;
    } else if (lighttype === 70) {
      // EnOcean
      // mParams.groupcode=$("#dialog-addmanuallightdevice #lightingparams_enocean #comboid option:selected").val();
      // mParams.unitcode=$("#dialog-addmanuallightdevice #lightingparams_enocean #combounitcode option:selected").val();
      mParams.groupcode = this.enoceanunitcode;
      mParams.unitcode = this.enoceanid;
      const ID = 'EnOcean';
      mParams.id = ID;
    } else if (lighttype === 68) {
      // GPIO
      mParams.id = 'GPIO';
      mParams.unitcode = this.gpio;
    } else if (lighttype === 69) {
      // SysfsGpio
      const ID =
        this.lighting2cmd1 + this.lighting2cmd2 + this.lighting2cmd3 + this.lighting2cmd4;
      mParams.id = ID;
      mParams.unitcode = this.sysfsgpio;
    } else if ((lighttype < 20) || (lighttype === 101)) {
      mParams.housecode = this.lighting1housecode;
      mParams.unitcode = this.lighting1unitcode;
    } else if (lighttype === 104) {
      mParams.unitcode = this.he105unitcode;
    } else if ((lighttype >= 200) && (lighttype < 300)) {
      // Blinds
      const ID = this.hexaValues.find(_ => _.val === this.blindscombocmd1).text +
        this.hexaValues.find(_ => _.val === this.blindscombocmd2).text +
        this.hexaValues.find(_ => _.val === this.blindscombocmd3).text +
        '0' + this.hexaValues.find(_ => _.val === this.blindscombocmd4).text;
      mParams.id = ID;
      mParams.unitcode = this.blindsunitcode;
    } else if (lighttype === 302) {
      // Home Confort
      const ID = this.hexaValues.find(_ => _.val === this.homeconfortcmd1).text +
        this.hexaValues.find(_ => _.val === this.homeconfortcmd2).text +
        this.hexaValues.find(_ => _.val === this.homeconfortcmd3).text;
      mParams.id = ID;
      mParams.housecode = this.homeconforthousecode;
      mParams.unitcode = this.homeconfortunitcode;
    } else if (lighttype === 304) {
      // Fan (Itho)
      const ID = this.fancmd1 + this.fancmd2 + this.fancmd3;
      mParams.id = ID;
    } else if ((lighttype === 305) || (lighttype === 306) || (lighttype === 307)) {
      // Fan (Lucci Air/Westinghouse)
      const ID = this.fancmd1 + this.fancmd2 + this.fancmd3;
      mParams.id = ID;
    } else if (lighttype === 400) {
      // OpenWebNet Bus Blinds
      const appID = parseInt(this.openwebnetcmd1 + this.openwebnetcmd2);
      const ID = ('002' + ('0000' + appID.toString(16)).slice(-4)); // WHO_AUTOMATION
      const unitcode = this.openwebnetcmd3;
      mParams.id = ID.toUpperCase();
      mParams.unitcode = unitcode;
    } else if (lighttype === 401) {
      // OpenWebNet Bus Lights
      const appID = parseInt(this.openwebnetcmd1 + this.openwebnetcmd2);
      const ID = ('001' + ('0000' + appID.toString(16)).slice(-4)); //  WHO_LIGHTING
      const unitcode = this.openwebnetcmd3;
      mParams.id = ID.toUpperCase();
      mParams.unitcode = unitcode;
    } else if (lighttype === 402) {
      // OpenWebNet Bus Auxiliary
      const appID = parseInt(this.openwebnetAUXcmd1);
      const ID = ('009' + ('0000' + appID.toString(16)).slice(-4)); //  WHO_AUXILIARY
      const unitcode = '0';
      mParams.id = ID.toUpperCase();
      mParams.unitcode = unitcode;
    } else if (lighttype === 403) {
      // OpenWebNet Zigbee Blinds
      const ID = this.zigbeecmd1;
      if (parseInt(ID) <= 0 || parseInt(ID) >= 0xFFFFFFFF) {
        this.notificationService.ShowNotify(this.translationService.t('Zigbee id is incorrect!'), 2500, true);
        return '';
      }
      const unitcode = this.zigbeecmd2;
      mParams.id = ID;
      mParams.unitcode = unitcode;
    } else if (lighttype === 404) {
      // OpenWebNet Zigbee Light
      const ID = this.zigbeecmd1;
      if (parseInt(ID) <= 0 || parseInt(ID) >= 0xFFFFFFFF) {
        this.notificationService.ShowNotify(this.translationService.t('Zigbee id is incorrect!'), 2500, true);
        return '';
      }
      const unitcode = this.zigbeecmd2;
      mParams.id = ID;
      mParams.unitcode = unitcode;
    } else if (lighttype === 405) {
      // OpenWebNet Dry Contact
      const appID = parseInt(this.openwebnetdrycontactcmd1);
      const ID = ('0019' + ('0000' + appID.toString(16)).slice(-4)); // WHO_DRY_CONTACT_IR_DETECTION (25 = 0x19)
      const unitcode = '0';
      mParams.id = ID.toUpperCase();
      mParams.unitcode = unitcode;
    } else if (lighttype === 406) {
      // OpenWebNet IR Detection
      const appID = parseInt(this.openwebnetIRdeteccmd1 + this.openwebnetIRdeteccmd2);
      const ID = ('0019' + ('0000' + appID.toString(16)).slice(-4)); // WHO_DRY_CONTACT_IR_DETECTION (25 = 0x19)
      const unitcode = '0';
      mParams.id = ID.toUpperCase();
      mParams.unitcode = unitcode;
    } else if (lighttype === 407) {
      // Openwebnet Bus Custom
      const appID = Number(this.openwebnetcustomcmd1);
      const ID = ('F00' + ('0000' + appID.toString(16)).slice(-4));
      const unitcode = this.openwebnetcustomcmd2;

      // var intRegex = /^[0-9*#]$/;
      // if (!intRegex.test(cmdText)) {
      //    ShowNotify($.t('Open command error. Please use only number, \'*\' or \'#\''), 2500, true);
      //    return "";
      // }
      mParams.id = ID;
      mParams.unitcode = unitcode;
      mParams.StrParam1 = this.openwebnetcustominputcmd1;
    } else {
      // AC
      let ID = '';
      let bIsType5 = 0;
      if (
        (lighttype === 50) ||
        (lighttype === 55) ||
        (lighttype === 57) ||
        (lighttype === 65) ||
        (lighttype === 100) ||
        (lighttype === 102) ||
        (lighttype === 103) ||
        (lighttype === 105) ||
        (lighttype === 107)
      ) {
        bIsType5 = 1;
      }
      if (bIsType5 === 0) {
        ID = this.lighting2cmd1 + this.lighting2cmd2 + this.lighting2cmd3 + this.lighting2cmd4;
      } else {
        if ((lighttype !== 57) && (lighttype !== 100)) {
          ID = this.lighting2cmd2 + this.lighting2cmd3 + this.lighting2cmd4;
        } else {
          ID = this.lighting2cmd3 + this.lighting2cmd4;
        }
      }
      if (ID === '') {
        this.notificationService.ShowNotify(this.translationService.t('Invalid Switch ID!'), 2500, true);
        return '';
      }
      mParams.id = ID;
      mParams.unitcode = this.lighting2unitcode;
    }

    const bIsSubDevice = this.how === 'Sub Device';
    let MainDeviceIdx = '';
    if (bIsSubDevice) {
      MainDeviceIdx = this.subdevice;
      if (typeof MainDeviceIdx === 'undefined') {
        bootbox.alert(this.translationService.t('No Main Device Selected!'));
        return '';
      }
    }
    if (MainDeviceIdx !== '') {
      mParams.maindeviceidx = MainDeviceIdx;
    }

    return mParams;
  }


}
