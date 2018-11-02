import {Injectable} from '@angular/core';
import * as Blockly from 'blockly';
import {DeviceService} from '../../_shared/_services/device.service';
import {SceneService} from '../../_shared/_services/scene.service';
import {CamService} from '../../_shared/_services/cam.service';
import {UserVariablesService} from '../../_shared/_services/user-variables.service';
import {DomoticzBlocklyMsg} from './blockly-msg';
import {Observable, of} from 'rxjs';
import {mergeMap, tap} from 'rxjs/operators';

@Injectable()
export class BlocklyService {

  private initialized = false;

  constructor(private deviceService: DeviceService,
              private sceneService: SceneService,
              private camService: CamService,
              private userVariablesService: UserVariablesService) {
  }

  init(): Observable<any> {
    if (!this.initialized) {

      Blockly.Msg['SWITCH_HUE'] = '30';
      Blockly.Msg['LOGIC_HUE'] = '120';

      // Sequential loading
      return of(undefined).pipe(
        mergeMap(() => this.initSwitchesBlocks()),
        mergeMap(() => this.initTempBlocks()),
        mergeMap(() => this.initWeatherBlocks()),
        mergeMap(() => this.initUtilityBlocks()),
        mergeMap(() => this.initZWaveAlarmsBlocks()),
        mergeMap(() => this.initScenesBlocks()),
        mergeMap(() => this.initCamerasBlocks()),
        mergeMap(() => this.initUserVariablesBlocks()),
        tap(() => {
          this.initControlBlocks();
          this.initLogicBlocks();
          this.initSendBlocks();
          this.initSecurityBlocks();
          this.initOtherBlocks();
        }),
        tap(() => {
          this.initialized = true;
        })
      );
    } else {
      return of(undefined);
    }
  }

  private initSwitchesBlocks(): Observable<any> {
    return this.deviceService.getDevices('light', true, 'Name', undefined, undefined, undefined, undefined, true)
      .pipe(
        tap(data => {
          const switchesAF: Array<BlocklyStuff> = [];
          const switchesGL: Array<BlocklyStuff> = [];
          const switchesMR: Array<BlocklyStuff> = [];
          const switchesSZ: Array<BlocklyStuff> = [];

          if (typeof data.result !== 'undefined') {
            data.result.forEach((item, i) => {
              if ('ghijkl'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
                switchesGL.push([item.Name, item.idx]);
              } else if ('mnopqr'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
                switchesMR.push([item.Name, item.idx]);
              } else if ('stuvwxyz'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
                switchesSZ.push([item.Name, item.idx]);
              } else {
                switchesAF.push([item.Name, item.idx]);
              }
            });
          }

          if (switchesAF.length === 0) {
            switchesAF.push(['No devices found', '0']);
          }
          if (switchesGL.length === 0) {
            switchesGL.push(['No devices found', '0']);
          }
          if (switchesMR.length === 0) {
            switchesMR.push(['No devices found', '0']);
          }
          if (switchesSZ.length === 0) {
            switchesSZ.push(['No devices found', '0']);
          }

          switchesAF.sort();
          switchesGL.sort();
          switchesMR.sort();
          switchesSZ.sort();

          // @ts-ignore
          Blockly.Blocks['switchvariablesAF'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(Blockly.Msg['SWITCH_HUE']);
              this.appendDummyInput()
                .appendField('A-F ')
                .appendField(new Blockly.FieldDropdown(switchesAF), 'Switch');
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZSWITCHES_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['switchvariablesGL'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(Blockly.Msg['SWITCH_HUE']);
              this.appendDummyInput()
                .appendField('G-L ')
                .appendField(new Blockly.FieldDropdown(switchesGL), 'Switch');
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZSWITCHES_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['switchvariablesMR'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(Blockly.Msg['SWITCH_HUE']);
              this.appendDummyInput()
                .appendField('M-R ')
                .appendField(new Blockly.FieldDropdown(switchesMR), 'Switch');
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZSWITCHES_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['switchvariablesSZ'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(Blockly.Msg['SWITCH_HUE']);
              this.appendDummyInput()
                .appendField('S-Z ')
                .appendField(new Blockly.FieldDropdown(switchesSZ), 'Switch');
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZSWITCHES_TOOLTIP);
            }
          };
        })
      );
  }

  private initTempBlocks(): Observable<any> {
    return this.deviceService.getDevices('temp', true, 'Name', undefined, undefined, undefined, undefined, true)
      .pipe(
        tap(data => {
          const temperatures: Array<BlocklyStuff> = [];
          const humidity: Array<BlocklyStuff> = [];
          const dewpoint: Array<BlocklyStuff> = [];
          const barometer: Array<BlocklyStuff> = [];

          if (typeof data.result !== 'undefined') {
            data.result.forEach((item, i) => {
              if (item.Type.toLowerCase().indexOf('temp') >= 0) {
                temperatures.push([item.Name, item.idx]);
              }
              if ((item.Type === 'RFXSensor') && (item.SubType === 'Temperature')) {
                temperatures.push([item.Name, item.idx]);
              }
              if (item.Type.toLowerCase().indexOf('hum') >= 0) {
                humidity.push([item.Name, item.idx]);
              }
              if (item.Type.toLowerCase().indexOf('baro') >= 0) {
                barometer.push([item.Name, item.idx]);
              }
              if (typeof item.DewPoint !== 'undefined') {
                dewpoint.push([item.Name, item.idx]);
              }
            });
          }

          if (temperatures.length === 0) {
            temperatures.push(['No temperatures found', '0']);
          }
          if (humidity.length === 0) {
            humidity.push(['No humidity found', '0']);
          }
          if (dewpoint.length === 0) {
            dewpoint.push(['No dew points found', '0']);
          }
          if (barometer.length === 0) {
            barometer.push(['No barometer found', '0']);
          }

          temperatures.sort();
          humidity.sort();
          dewpoint.sort();
          barometer.sort();

          // @ts-ignore
          Blockly.Blocks['temperaturevariables'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(330);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(temperatures), 'Temperature');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_TEMP, 'TemperatureLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_TEMPERATURE_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['humidityvariables'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(226);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(humidity), 'Humidity');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_HUM, 'HumidityLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_HUMIDITY_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['dewpointvariables'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(330);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(dewpoint), 'Dewpoint');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_DEWPOINT, 'DewpointLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_DEWPOINT_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['barometervariables'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(68);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(barometer), 'Barometer');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_BARO, 'BarometerLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_BAROMETER_TOOLTIP);
            }
          };
        })
      );
  }

  private initWeatherBlocks(): Observable<any> {
    return this.deviceService.getDevices('weather', true, 'Name', undefined, undefined, undefined, undefined, true)
      .pipe(
        tap(data => {
          const weather: Array<BlocklyStuff> = [];

          if (typeof data.result !== 'undefined') {
            data.result.forEach((item, i) => {
              weather.push([item.Name, item.idx]);
            });
          }

          if (weather.length === 0) {
            weather.push(['No weather found', '0']);
          }

          weather.sort();

          // @ts-ignore
          Blockly.Blocks['weathervariables'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(210);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(weather), 'Weather');
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_WEATHER_TOOLTIP);
            }
          };
        })
      );
  }

  private initUtilityBlocks(): Observable<any> {
    return this.deviceService.getDevices('utility', true, 'Name', undefined, undefined, undefined, undefined, true)
      .pipe(
        tap(data => {

          const utilities: Array<BlocklyStuff> = [];
          const utilitiesAF: Array<BlocklyStuff> = [];
          const utilitiesGL: Array<BlocklyStuff> = [];
          const utilitiesMR: Array<BlocklyStuff> = [];
          const utilitiesSZ: Array<BlocklyStuff> = [];
          const texts: Array<BlocklyStuff> = [];
          const setpoints: Array<BlocklyStuff> = [];

          if (typeof data.result !== 'undefined') {
            data.result.forEach((item, i) => {
              if (item.SubType === 'Text') {
                texts.push([item.Name, item.idx]);
              } else {
                utilities.push([item.Name, item.idx]);
                if ('ghijkl'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
                  utilitiesGL.push([item.Name, item.idx]);
                } else if ('mnopqr'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
                  utilitiesMR.push([item.Name, item.idx]);
                } else if ('stuvwxyz'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
                  utilitiesSZ.push([item.Name, item.idx]);
                } else {
                  utilitiesAF.push([item.Name, item.idx]);
                }
                if (item.SubType === 'SetPoint') {
                  setpoints.push([item.Name, item.idx]);
                }
              }
            });
          }

          if (utilities.length === 0) {
            utilities.push(['No utilities found', '0']);
          }
          if (utilitiesAF.length === 0) {
            utilitiesAF.push(['No utilities found', '0']);
          }
          if (utilitiesGL.length === 0) {
            utilitiesGL.push(['No utilities found', '0']);
          }
          if (utilitiesMR.length === 0) {
            utilitiesMR.push(['No utilities found', '0']);
          }
          if (utilitiesSZ.length === 0) {
            utilitiesSZ.push(['No utilities found', '0']);
          }
          if (texts.length === 0) {
            texts.push(['No text devices found', '0']);
          }
          if (setpoints.length === 0) {
            setpoints.push(['No SetPoints found', '0']);
          }

          utilities.sort();
          utilitiesAF.sort();
          utilitiesGL.sort();
          utilitiesMR.sort();
          utilitiesSZ.sort();
          texts.sort();
          setpoints.sort();

          // @ts-ignore
          Blockly.Blocks['utilityvariables'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(330);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(utilities), 'Utility');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['utilityvariablesAF'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(290);
              this.appendDummyInput()
                .appendField('A-F ')
                .appendField(new Blockly.FieldDropdown(utilitiesAF), 'Utility');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['utilityvariablesGL'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(290);
              this.appendDummyInput()
                .appendField('G-L ')
                .appendField(new Blockly.FieldDropdown(utilitiesGL), 'Utility');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['utilityvariablesMR'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(290);
              this.appendDummyInput()
                .appendField('M-R ')
                .appendField(new Blockly.FieldDropdown(utilitiesMR), 'Utility');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['utilityvariablesSZ'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(290);
              this.appendDummyInput()
                .appendField('S-Z ')
                .appendField(new Blockly.FieldDropdown(utilitiesSZ), 'Utility');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['setpointvariables'] = {
            // Variable getter.
            category: null,  // Variables are handled specially.
            init: function () {
              this.setColour(100);
              this.setPreviousStatement(true);
              this.setNextStatement(true);

              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SETSETPOINT)
                .appendField(new Blockly.FieldDropdown(setpoints), 'SetPoint')
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_TO)
                .appendField(new Blockly.FieldTextInput('0'), 'NUM')
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_DEGREES);
              this.setInputsInline(true);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SETPOINT_TOOLTIP);
            }
          };

          // @ts-ignore
          Blockly.Blocks['textvariables'] = {
            // Variable getter.
            category: null,
            init: function () {
              this.setColour(330);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(texts), 'Text');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_TEXT, 'TextLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_TEXT_TOOLTIP);
            }
          };

        })
      );
  }

  private initZWaveAlarmsBlocks(): Observable<any> {
    return this.deviceService.getDevices('zwavealarms', true, 'Name', undefined, undefined, undefined, undefined, true)
      .pipe(
        tap(data => {
          const zwavealarms: Array<BlocklyStuff> = [];

          if (typeof data.result !== 'undefined') {
            data.result.forEach((item, i) => {
              zwavealarms.push([item.Name, item.idx]);
            });
          }

          if (zwavealarms.length === 0) {
            zwavealarms.push(['No ZWave Alarms found', '0']);
          }

          // @ts-ignore
          Blockly.Blocks['zwavealarms'] = {
            // Variable getter.
            category: null,
            init: function () {
              this.setColour(330);
              this.appendDummyInput()
                .appendField(new Blockly.FieldDropdown(zwavealarms), 'ZWave Alarms');
              this.appendDummyInput()
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_ZWAVEALARMS, 'ZWaveAlarmsLabel');
              this.setInputsInline(true);
              this.setOutput(true, null);
              this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_ZWAVEALARMS_TOOLTIP);
            }
          };

        })
      );
  }

  private initScenesBlocks(): Observable<any> {
    return this.sceneService.getScenesForBlockly('Name', true).pipe(
      tap(data => {

        const scenes: Array<BlocklyStuff> = [];
        const groups: Array<BlocklyStuff> = [];

        if (typeof data.result !== 'undefined') {
          data.result.forEach((item, i) => {
            if (item.Type === 'Scene') {
              scenes.push([item.Name, item.idx]);
            } else if (item.Type === 'Group') {
              groups.push([item.Name, item.idx]);
            }
          });
        }

        if (groups.length === 0) {
          groups.push(['No groups found', '0']);
        }
        if (scenes.length === 0) {
          scenes.push(['No scenes found', '0']);
        }

        groups.sort();
        scenes.sort();

        // @ts-ignore
        Blockly.Blocks['scenevariables'] = {
          // Variable getter.
          category: null,  // Variables are handled specially.
          init: function () {
            this.setColour(200);
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.appendDummyInput()
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SET)
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SCENE)
              .appendField(new Blockly.FieldDropdown(scenes), 'Scene')
              .appendField(' = ')
              .appendField(new Blockly.FieldDropdown(this.STATE), 'Status');
            this.setInputsInline(true);
            this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_SCENES_TOOLTIP);
          }
        };

        // @ts-ignore
        Blockly.Blocks['scenevariables'].STATE =
          [['Active', 'Active'],
            ['Inactive', 'Inactive'],
            ['On', 'On']];

        // @ts-ignore
        Blockly.Blocks['groupvariables'] = {
          // Variable getter.
          category: null,  // Variables are handled specially.
          init: function () {
            this.setColour(200);
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.appendDummyInput()
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SET)
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_GROUP)
              .appendField(new Blockly.FieldDropdown(groups), 'Group')
              .appendField(' = ')
              .appendField(new Blockly.FieldDropdown(this.STATE), 'Status');
            this.setInputsInline(true);
            this.setTooltip(DomoticzBlocklyMsg.DOMOTICZVARIABLES_GROUPS_TOOLTIP);
          }
        };

        // @ts-ignore
        Blockly.Blocks['groupvariables'].STATE =
          [['Active', 'Active'],
            ['Inactive', 'Inactive'],
            ['On', 'On'],
            ['Off', 'Off']];
      })
    );
  }

  private initCamerasBlocks(): Observable<any> {
    return this.camService.getCamerasForBlockly('Name', true).pipe(
      tap(data => {

        const cameras: Array<BlocklyStuff> = [];

        if (typeof data.result !== 'undefined') {
          data.result.forEach((item, i) => {
            cameras.push([item.Name, item.idx]);
          });
        }

        if (cameras.length === 0) {
          cameras.push(['No cameras found', '0']);
        }

        cameras.sort();

        // @ts-ignore
        Blockly.Blocks['cameravariables'] = {
          // Variable getter.
          category: null,  // Variables are handled specially.
          init: function () {
            this.setColour(100);
            this.setPreviousStatement(true);
            this.setNextStatement(true);

            this.appendDummyInput()
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SENDCAMERA)
              .appendField(new Blockly.FieldDropdown(cameras), 'Camera')
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_WITHSUBJECT)
              .appendField(new Blockly.FieldTextInput(''), 'Subject')
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_AFTER)
              .appendField(new Blockly.FieldTextInput('0'), 'NUM')
              .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SECONDS);
            this.setInputsInline(true);
            this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_CAMERA_TOOLTIP);
          }
        };
      })
    );
  }

  private initUserVariablesBlocks(): Observable<any> {
    return this.userVariablesService.getUserVariables().pipe(
      tap(data => {

        const variablesAF: Array<BlocklyStuff> = [];
        const variablesGL: Array<BlocklyStuff> = [];
        const variablesMR: Array<BlocklyStuff> = [];
        const variablesSZ: Array<BlocklyStuff> = [];

        if (typeof data.result !== 'undefined') {
          data.result.forEach((item, i) => {
            if ('ghijkl'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
              variablesGL.push([item.Name, item.idx]);
            } else if ('mnopqr'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
              variablesMR.push([item.Name, item.idx]);
            } else if ('stuvwxyz'.indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
              variablesSZ.push([item.Name, item.idx]);
            } else {
              variablesAF.push([item.Name, item.idx]);
            }
          });
        }

        if (variablesAF.length === 0) {
          variablesAF.push(['No devices found', '0']);
        }
        if (variablesGL.length === 0) {
          variablesGL.push(['No devices found', '0']);
        }
        if (variablesMR.length === 0) {
          variablesMR.push(['No devices found', '0']);
        }
        if (variablesSZ.length === 0) {
          variablesSZ.push(['No devices found', '0']);
        }

        variablesAF.sort();
        variablesGL.sort();
        variablesMR.sort();
        variablesSZ.sort();

        // @ts-ignore
        Blockly.Blocks['uservariablesAF'] = {
          // Variable getter.
          category: null,  // Variables are handled specially.
          init: function () {
            this.setColour(Blockly.Msg['VARIABLES_HUE']);
            this.appendDummyInput()
              .appendField('Var A-F ')
              .appendField(new Blockly.FieldDropdown(variablesAF), 'Variable');
            this.setOutput(true, null);
            this.setTooltip(DomoticzBlocklyMsg.DOMOTICZUSERVARIABLES_TOOLTIP);
          }
        };

        // @ts-ignore
        Blockly.Blocks['uservariablesGL'] = {
          // Variable getter.
          category: null,  // Variables are handled specially.
          init: function () {
            this.setColour(Blockly.Msg['VARIABLES_HUE']);
            this.appendDummyInput()
              .appendField('Var G-L ')
              .appendField(new Blockly.FieldDropdown(variablesGL), 'Variable');
            this.setOutput(true, null);
            this.setTooltip(DomoticzBlocklyMsg.DOMOTICZUSERVARIABLES_TOOLTIP);
          }
        };

        // @ts-ignore
        Blockly.Blocks['uservariablesMR'] = {
          // Variable getter.
          category: null,  // Variables are handled specially.
          init: function () {
            this.setColour(Blockly.Msg['VARIABLES_HUE']);
            this.appendDummyInput()
              .appendField('Var M-R ')
              .appendField(new Blockly.FieldDropdown(variablesMR), 'Variable');
            this.setOutput(true, null);
            this.setTooltip(DomoticzBlocklyMsg.DOMOTICZUSERVARIABLES_TOOLTIP);
          }
        };

        // @ts-ignore
        Blockly.Blocks['uservariablesSZ'] = {
          // Variable getter.
          category: null,  // Variables are handled specially.
          init: function () {
            this.setColour(Blockly.Msg['VARIABLES_HUE']);
            this.appendDummyInput()
              .appendField('Var S-Z ')
              .appendField(new Blockly.FieldDropdown(variablesSZ), 'Variable');
            this.setOutput(true, null);
            this.setTooltip(DomoticzBlocklyMsg.DOMOTICZUSERVARIABLES_TOOLTIP);
          }
        };
      })
    );
  }

  private initControlBlocks(): void {
    // @ts-ignore
    Blockly.Blocks['domoticzcontrols_if'] = {
      category: null,  // Variables are handled specially.
      init: function () {
        this.setColour(Blockly.Msg['LOGIC_HUE']);
        this.appendValueInput('IF0')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_IF);
        this.appendStatementInput('DO0')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_DO);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZCONTROLS_IF_TOOLTIP);
        // this.setPreviousStatement(true);
        // this.setNextStatement(true);
      }
    };

    // @ts-ignore
    Blockly.Blocks['domoticzcontrols_ifelseif'] = {
      // If/elseif/else condition.
      // @ts-ignore
      helpUrl: Blockly.LANG_CONTROLS_IF_HELPURL,
      init: function () {
        this.setColour(Blockly.Msg['LOGIC_HUE']);
        this.appendValueInput('IF0')
          .setCheck('Boolean')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_IF);
        this.appendStatementInput('DO0')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_DO);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setMutator(new Blockly.Mutator(['controls_if_elseif']));
        // Assign 'this' to a variable for use in the tooltip closure below.
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZCONTROLS_IF_TOOLTIP);
        this.elseifCount_ = 0;
        this.elseCount_ = 0;
      },
      mutationToDom: function () {
        if (!this.elseifCount_) {
          return null;
        }
        const container = document.createElement('mutation');
        if (this.elseifCount_) {
          container.setAttribute('elseif', this.elseifCount_);
        }
        return container;
      },
      domToMutation: function (xmlElement) {
        this.elseifCount_ = parseInt(xmlElement.getAttribute('elseif'), 10) || 0;
        for (let x = 1; x <= this.elseifCount_; x++) {
          this.appendValueInput('IF' + x)
            .setCheck('Boolean')
            .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_ELSEIF);
          this.appendStatementInput('DO' + x)
            .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_DO);
        }
      },
      decompose: function (workspace) {
        const containerBlock = workspace.newBlock('controls_if_if');
        containerBlock.initSvg();
        let connection = containerBlock.nextConnection;
        for (let x = 1; x <= this.elseifCount_; x++) {
          const elseifBlock = workspace.newBlock('controls_if_elseif');
          elseifBlock.initSvg();
          connection.connect(elseifBlock.previousConnection);
          connection = elseifBlock.nextConnection;
        }
        return containerBlock;
      },
      compose: function (containerBlock) {
        // Disconnect all the elseif input blocks and remove the inputs.
        for (let x = this.elseifCount_; x > 0; x--) {
          this.removeInput('IF' + x);
          this.removeInput('DO' + x);
        }
        this.elseifCount_ = 0;
        // Rebuild the block's optional inputs.
        let clauseBlock = containerBlock.nextConnection.targetBlock();
        while (clauseBlock) {
          switch (clauseBlock.type) {
            case 'controls_if_elseif':
              this.elseifCount_++;
              const ifInput = this.appendValueInput('IF' + this.elseifCount_)
                .setCheck('Boolean')
                .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_ELSEIF);
              const doInput = this.appendStatementInput('DO' + this.elseifCount_);
              doInput.appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_DO);
              // Reconnect any child blocks.
              if (clauseBlock.valueConnection_) {
                ifInput.connection.connect(clauseBlock.valueConnection_);
              }
              if (clauseBlock.statementConnection_) {
                doInput.connection.connect(clauseBlock.statementConnection_);
              }
              break;
            default:
              throw new Error('Unknown block type.');
          }
          clauseBlock = clauseBlock.nextConnection &&
            clauseBlock.nextConnection.targetBlock();
        }
      },
      saveConnections: function (containerBlock) {
        // Store a pointer to any connected child blocks.
        let clauseBlock = containerBlock.nextConnection.targetBlock();
        let x = 1;
        while (clauseBlock) {
          switch (clauseBlock.type) {
            case 'controls_if_elseif':
              const inputIf = this.getInput('IF' + x);
              const inputDo = this.getInput('DO' + x);
              clauseBlock.valueConnection_ =
                inputIf && inputIf.connection.targetConnection;
              clauseBlock.statementConnection_ =
                inputDo && inputDo.connection.targetConnection;
              x++;
              break;
            default:
              throw new Error('Unknown block type.');
          }
          clauseBlock = clauseBlock.nextConnection &&
            clauseBlock.nextConnection.targetBlock();
        }
      }
    };
  }

  private initLogicBlocks(): void {

    // @ts-ignore
    Blockly.Blocks['logic_states'] = {
      helpUrl: null,
      init: function () {
        this.setColour(Blockly.Msg['MATH_HUE']);
        this.setOutput(true, null);
        this.appendDummyInput()
          .appendField(new Blockly.FieldDropdown(this.STATES), 'State');
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_set'] = {
      // Comparison operator.
      init: function () {
        this.setColour(Blockly.Msg['LOGIC_HUE']);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('A')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SET);
        this.appendValueInput('B')
          .appendField('=');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SET_TOOLTIP);

      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_setafter'] = {
      // Comparison operator.
      init: function () {
        this.setColour(Blockly.Msg['LOGIC_HUE']);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('A')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SET);
        this.appendValueInput('B')
          .appendField('=');
        this.appendValueInput('C')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_AFTER);
        this.appendDummyInput()
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SECONDS);
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SETAFTER_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_setdelayed'] = {
      // Comparison operator.
      init: function () {
        this.setColour(Blockly.Msg['LOGIC_HUE']);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('A')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SET);
        this.appendValueInput('B')
          .appendField('=');
        this.appendValueInput('C')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_FOR);
        this.appendDummyInput()
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_MINUTES);
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SETDELAYED_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_setrandom'] = {
      // Comparison operator.
      init: function () {
        this.setColour(Blockly.Msg['LOGIC_HUE']);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('A')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SET);
        this.appendValueInput('B')
          .appendField('=');
        this.appendValueInput('C')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_RANDOM);
        this.appendDummyInput()
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_MINUTES);
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SETRANDOM_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_setlevel'] = {
      init: function () {
        this.setColour(Blockly.Msg['MATH_HUE']);
        this.appendDummyInput()
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SETLEVEL);
        this.appendDummyInput()
          .appendField(new Blockly.FieldTextInput('0',
            this.percentageValidator), 'NUM');
        this.setOutput(true, 'Number');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SETLEVEL_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_setlevel'].percentageValidator = function (text) {
      let n = parseFloat(text || 0);
      if (!isNaN(n)) {
        if (n > 100) {
          n = 100;
        }
        if (n < 0) {
          n = 0;
        }
      }

      return isNaN(n) ? null : String(n);
    };

    // @ts-ignore
    Blockly.Blocks['logic_timeofday'] = {
      // Comparison operator.
      init: function () {
        this.setColour(Blockly.Msg['MATH_HUE']);
        this.setOutput(true, null);
        this.appendValueInput(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_TIME)
          .appendField('Time:')
          .appendField(new Blockly.FieldDropdown(this.OPERATORS), 'OP');
        this.setInputsInline(true);
        const thisBlock = this;
        this.setTooltip(function () {
          const op = thisBlock.getTitleValue('OP');
          return thisBlock.TOOLTIPS[op];
        });
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_timevalue'] = {
      init: function () {
        this.setColour(Blockly.Msg['MATH_HUE']);
        this.appendDummyInput()
          .appendField(new Blockly.FieldTextInput('00:00',
            this.TimeValidator), 'TEXT');
        this.setOutput(true, 'String');
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_TIMEVALUE_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_timevalue.TimeValidator'] = function (text) {
      if (text.match(/^([01]?[0-9]|2[0-3]):[0-5][0-9]/)) {
        return text;
      }
      return '00:00';
    };

    // @ts-ignore
    Blockly.Blocks['logic_timeofday'].TOOLTIPS = {
      EQ: DomoticzBlocklyMsg.LANG_LOGIC_COMPARE_TOOLTIP_EQ,
      NEQ: DomoticzBlocklyMsg.LANG_LOGIC_COMPARE_TOOLTIP_NEQ,
      LT: DomoticzBlocklyMsg.LANG_LOGIC_COMPARE_TOOLTIP_LT,
      LTE: DomoticzBlocklyMsg.LANG_LOGIC_COMPARE_TOOLTIP_LTE,
      GT: DomoticzBlocklyMsg.LANG_LOGIC_COMPARE_TOOLTIP_GT,
      GTE: DomoticzBlocklyMsg.LANG_LOGIC_COMPARE_TOOLTIP_GTE
    };

    // @ts-ignore
    Blockly.Blocks['logic_sunrisesunset'] = {
      init: function () {
        this.setOutput(true, null);
        this.setColour(Blockly.Msg['MATH_HUE']);
        this.appendDummyInput()
          .appendField(new Blockly.FieldDropdown(this.VALUES), 'SunriseSunset')
          .appendField(' ');
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SUNRISESUNSET_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_sunrisesunset'].VALUES =
      [['Sunrise', 'Sunrise'],
        ['Sunset', 'Sunset']];

    // @ts-ignore
    Blockly.Blocks['logic_notification_priority'] = {
      init: function () {
        this.setOutput(true, null);
        this.setColour(230);
        this.appendDummyInput()
          .appendField(new Blockly.FieldDropdown(this.PRIORITY), 'NotificationPriority')
          .appendField(' ');
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_NOTIFICATION_PRIORITY_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_weekday'] = {
      // Variable getter.
      init: function () {
        this.setColour(Blockly.Msg['MATH_HUE']);
        this.setOutput(true, null);
        this.appendDummyInput()
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_DAY)
          .appendField(new Blockly.FieldDropdown(this.OPERATORS), 'OP')
          .appendField(' ')
          .appendField(new Blockly.FieldDropdown(this.DAYS), 'Weekday');
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_WEEKDAY_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['logic_weekday'].DAYS =
      [['Monday', '2'],
        ['Tuesday', '3'],
        ['Wednesday', '4'],
        ['Thursday', '5'],
        ['Friday', '6'],
        ['Saturday', '7'],
        ['Sunday', '1']];

    // @ts-ignore
    Blockly.Blocks['logic_notification_priority'].PRIORITY =
      [['-2 (Prowl: Very Low, Pushover: N/A)', '-2'],
        ['-1 (Prowl: Moderate, Pushover:Quiet)', '-1'],
        ['0 (All: Normal)', '0'],
        ['1 (All: High)', '1'],
        ['2 (Prowl: Emergency, Pushover: confirm)', '2']];

    // @ts-ignore
    Blockly.Blocks['logic_notification_priority'].SOUND =
      [['Pushover (default)', 'pushover'],
        ['Bike', 'bike'],
        ['Bugle', 'bugle'],
        ['Cash Register', 'cashregister'],
        ['Classical', 'classical'],
        ['Cash Register', 'cashregister'],
        ['Cosmic', 'cosmic'],
        ['Falling', 'falling'],
        ['Gamelan', 'gamelan'],
        ['Incoming', 'incoming'],
        ['Intermission', 'intermission'],
        ['Magic', 'magic'],
        ['Mechanical', 'mechanical'],
        ['Piano Bar', 'pianobar'],
        ['Siren', 'siren'],
        ['Space Alarm', 'spacealarm'],
        ['Tug Boat', 'tugboat'],
        ['Alien Alarm (long)', 'alien'],
        ['Climb (long)', 'climb'],
        ['Persistent (long)', 'persistent'],
        ['Pushover Echo (long)', 'echo'],
        ['Up Down (long)', 'updown'],
        ['None (silent)', 'none']];

    // @ts-ignore
    Blockly.Blocks['logic_notification_priority'].SUBSYSTEM =
      [
        ['all (default)', ''],
        ['gcm', 'gcm'],
        ['http', 'http'],
        ['kodi', 'kodi'],
        ['lms', 'lms'],
        ['prowl', 'prowl'],
        ['pushalot', 'pushalot'],
        ['pushbullet', 'pushbullet'],
        ['pushover', 'pushover'],
        ['pushsafer', 'pushsafer'],
        ['telegram', 'telegram']
      ];

    // @ts-ignore
    Blockly.Blocks['logic_states'].STATES =
      [
        ['On', 'On'],
        ['Off', 'Off'],
        ['Group On', 'Group On'],
        ['Group Off', 'Group Off'],
        ['Open', 'Open'],
        ['Closed', 'Closed'],
        ['Locked', 'Locked'],
        ['Unlocked', 'Unlocked'],
        ['Panic', 'Panic'],
        ['Panic End', 'Panic End'],
        ['Normal', 'Normal'],
        ['Alarm', 'Alarm'],
        ['Motion', 'Motion'],
        ['No Motion', 'No Motion'],
        ['Chime', 'Chime'],
        ['Stop', 'Stop'],
        ['Stopped', 'Stopped'],
        ['Video', 'Video'],
        ['Audio', 'Audio'],
        ['Photo', 'Photo'],
        ['Paused', 'Paused'],
        ['Playing', 'Playing'],
        ['1', '1'],
        ['2', '2'],
        ['3', '3'],
        ['timer', 'timer'],
        ['Disco Mode 1', 'Disco Mode 1'],
        ['Disco Mode 2', 'Disco Mode 2'],
        ['Disco Mode 3', 'Disco Mode 3'],
        ['Disco Mode 4', 'Disco Mode 4'],
        ['Disco Mode 5', 'Disco Mode 5'],
        ['Disco Mode 6', 'Disco Mode 6'],
        ['Disco Mode 7', 'Disco Mode 7'],
        ['Disco Mode 8', 'Disco Mode 8'],
        ['Disco Mode 9', 'Disco Mode 9'],
      ];

    // @ts-ignore
    Blockly.Blocks['logic_weekday'].OPERATORS =
      [['=', 'EQ'],
        ['\u2260', 'NEQ'],
        ['<', 'LT'],
        ['\u2264', 'LTE'],
        ['>', 'GT'],
        ['\u2265', 'GTE']];

    // @ts-ignore
    Blockly.Blocks['logic_timeofday'].OPERATORS =
      [['=', 'EQ'],
        ['\u2260', 'NEQ'],
        ['<', 'LT'],
        ['\u2264', 'LTE'],
        ['>', 'GT'],
        ['\u2265', 'GTE']];
  }

  private initSendBlocks(): void {

    // @ts-ignore
    Blockly.Blocks['send_notification'] = {
      // Comparison operator.
      init: function () {
        this.setColour(Blockly.Msg['LOGIC_HUE']);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('notificationTextSubject')
          .appendField('Send notification with subject:');
        this.appendValueInput('notificationTextBody')
          .appendField('and message:');
        this.appendDummyInput()
          .appendField('through subsystem:')
          // @ts-ignore
          .appendField(new Blockly.FieldDropdown(Blockly.Blocks['logic_notification_priority'].SUBSYSTEM), 'notificationSubsystem');
        this.appendDummyInput()
          .appendField('with priority:')
          // @ts-ignore
          .appendField(new Blockly.FieldDropdown(Blockly.Blocks['logic_notification_priority'].PRIORITY), 'notificationPriority');
        this.appendDummyInput()
          .appendField('with sound (Pushover):')
          // @ts-ignore
          .appendField(new Blockly.FieldDropdown(Blockly.Blocks['logic_notification_priority'].SOUND), 'notificationSound');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_NOTIFICATION_TOOLTIP);

      }
    };

    // @ts-ignore
    Blockly.Blocks['send_email'] = {
      // Comparison operator.
      init: function () {
        this.setColour(120);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendDummyInput()
          .appendField('Send email with subject:')
          .appendField(new Blockly.FieldTextInput(''), 'TextSubject');
        this.appendDummyInput()
          .appendField('and message:')
          .appendField(new Blockly.FieldTextInput(''), 'TextBody');
        this.appendDummyInput()
          .appendField('to:')
          .appendField(new Blockly.FieldTextInput(''), 'TextTo');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_EMAIL_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['send_sms'] = {
      // Comparison operator.
      init: function () {
        this.setColour(120);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendDummyInput()
          .appendField('Send SMS with subject:')
          .appendField(new Blockly.FieldTextInput(''), 'TextSubject');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SMS_TOOLTIP);
      }
    };
  }

  private initSecurityBlocks(): void {

    // @ts-ignore
    Blockly.Blocks['security_status'] = {
      // Variable getter.
      init: function () {
        this.setColour(0);
        this.setOutput(true, null);
        this.appendDummyInput()
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SECURITY)
          .appendField(new Blockly.FieldDropdown(this.OPERATORS), 'OP')
          .appendField(' ')
          .appendField(new Blockly.FieldDropdown(this.STATE), 'Status');
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_SECURITY_STATUS_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['security_status'].STATE =
      [['Disarmed', '0'],
        ['Armed Home', '1'],
        ['Armed Away', '2']];

    // @ts-ignore
    Blockly.Blocks['security_status'].OPERATORS =
      [['=', 'EQ'],
        ['\u2260', 'NEQ']];

  }

  private initOtherBlocks(): void {

    // @ts-ignore
    Blockly.Blocks['trigger_ifttt'] = {
      // Comparison operator.
      init: function () {
        this.setColour(120);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendDummyInput()
          .appendField('Trigger IFTTT. Event_ID:')
          .appendField(new Blockly.FieldTextInput(''), 'EventID');
        this.appendDummyInput()
          .appendField('Optional: value1:')
          .appendField(new Blockly.FieldTextInput(''), 'TextValue1');
        this.appendDummyInput()
          .appendField('value2:')
          .appendField(new Blockly.FieldTextInput(''), 'TextValue2');
        this.appendDummyInput()
          .appendField('value3:')
          .appendField(new Blockly.FieldTextInput(''), 'TextValue3');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_IFTTT_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['start_script'] = {
      // Comparison operator.
      init: function () {
        this.setColour(120);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendDummyInput()
          .appendField('Start Script:')
          .appendField(new Blockly.FieldTextInput(''), 'TextPath');
        this.appendDummyInput()
          .appendField('with parameter(s):')
          .appendField(new Blockly.FieldTextInput(''), 'TextParam');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_SCRIPT_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['open_url'] = {
      // Comparison operator.
      init: function () {
        this.setColour(120);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('urlToOpen')
          .appendField('Open URL:');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_OPENURL_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['open_url_after'] = {
      // Comparison operator.
      init: function () {
        this.setColour(120);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('urlToOpen')
          .appendField('Open URL:');
        this.setInputsInline(true);
        this.appendDummyInput()
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_AFTER)
          .appendField(new Blockly.FieldTextInput('0'), 'urlAfter')
          .appendField(DomoticzBlocklyMsg.DOMOTICZCONTROLS_MSG_SECONDS);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_OPENURL_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['url_text'] = {
      // Text value.
      init: function () {
        this.setColour(160);
        this.appendDummyInput()
          .appendField('http://')
          .appendField(new Blockly.FieldTextInput('', this.URLValidator), 'TEXT');
        this.setOutput(true, 'String');
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_URL_TEXT_TOOLTIP);
      }
    };

    // @ts-ignore
    Blockly.Blocks['url_text'].URLValidator = function (text) {
      if (text.substr(0, 7).toLowerCase() === 'http://') {
        text = text.substr(7);
      }
      return text;
    };

    // @ts-ignore
    Blockly.Blocks['writetolog'] = {
      // Comparison operator.
      init: function () {
        this.setColour(90);
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.appendValueInput('writeToLog')
          .appendField('Write to log:');
        this.setInputsInline(true);
        this.setTooltip(DomoticzBlocklyMsg.DOMOTICZ_LOGIC_WRITETOLOG_TOOLTIP);
      }
    };
  }

}

type BlocklyStuff = [string, string];
