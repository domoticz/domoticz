// FIXME proper declaration
declare var $: any;

export class BlocklyXmlParserService {

  constructor() {
  }

  private static GetValueText(value: any, variableType: string): string {
    if (variableType.indexOf('switchvariables') >= 0) {
      const fieldA = $(value).find('field')[0];
      return 'device[' + $(fieldA).text() + ']';
    } else if (variableType.indexOf('uservariables') >= 0) {
      const fieldA = $(value).find('field')[0];
      return 'variable[' + $(fieldA).text() + ']';
    } else if (variableType === 'temperaturevariables') {
      const fieldA = $(value).find('field')[0];
      return 'temperaturedevice[' + $(fieldA).text() + ']';
    } else if (variableType === 'humidityvariables') {
      const fieldA = $(value).find('field')[0];
      return 'humiditydevice[' + $(fieldA).text() + ']';
    } else if (variableType === 'dewpointvariables') {
      const fieldA = $(value).find('field')[0];
      return 'dewpointdevice[' + $(fieldA).text() + ']';
    } else if (variableType === 'barometervariables') {
      const fieldA = $(value).find('field')[0];
      return 'barometerdevice[' + $(fieldA).text() + ']';
    } else if (
      (variableType.indexOf('utilityvariables') >= 0) ||
      (variableType.indexOf('textvariables') >= 0)
    ) {
      const fieldA = $(value).find('field')[0];
      return 'utilitydevice[' + $(fieldA).text() + ']';
    } else if (variableType.indexOf('weathervariables') >= 0) {
      const fieldA = $(value).find('field')[0];
      return 'weatherdevice[' + $(fieldA).text() + ']';
    } else if (variableType.indexOf('zwavealarms') >= 0) {
      const fieldA = $(value).find('field')[0];
      return 'zwavealarms[' + $(fieldA).text() + ']';
    } else {
      const fieldB = $(value).find('field')[0];
      if ($(fieldB).attr('name') === 'State') {
        return '"' + $(fieldB).text() + '"';
      } else if ($(fieldB).attr('name') === 'TEXT') {
        return '"' + $(fieldB).text() + '"';
      } else if ($(fieldB).attr('name') === 'NUM') {
        return $(fieldB).text();
      }
      return 'unknown comparevariable ' + variableType;
    }
  }

  private static parseLogicCompare(thisBlock: any): string {
    const locOperand = BlocklyXmlParserService.opSymbol($($(thisBlock).children('field:first')).text());
    const valueA = $(thisBlock).children('value[name=\'A\']')[0];
    const variableTypeA = $(valueA).children('block:first').attr('type');
    const valueB = $(thisBlock).children('value[name=\'B\']')[0];
    const variableTypeB = $(valueB).children('block:first').attr('type');
    const varTextA = BlocklyXmlParserService.GetValueText(valueA, variableTypeA);
    const varTextB = BlocklyXmlParserService.GetValueText(valueB, variableTypeB);

    let compareString = varTextA;
    compareString += locOperand;
    compareString += varTextB;
    return compareString;
  }


  private static opSymbol(operand: string): string {
    switch (operand) {
      case 'EQ':
        operand = ' == ';
        break;
      case 'NEQ':
        operand = ' ~= ';
        break;
      case 'LT':
        operand = ' < ';
        break;
      case 'GT':
        operand = ' > ';
        break;
      case 'LTE':
        operand = ' <= ';
        break;
      case 'GTE':
        operand = ' >= ';
        break;
      default:
    }
    return operand;
  }

  private static parseLogicTimeOfDay(thisBlock: any): string {
    let compareString = '';
    const locOperand = BlocklyXmlParserService.opSymbol($($(thisBlock).children('field:first')).text());
    const valueTime = $(thisBlock).children('value[name=\'Time\']')[0];
    const timeBlock = $(valueTime).children('block:first');
    if (timeBlock.attr('type') === 'logic_timevalue') {
      const valueA = $(timeBlock).children('field[name=\'TEXT\']')[0];
      let totalminutes = 9999;
      const res = $(valueA).text().split(':');
      if (res.length === 2) {
        const hours = Number(res[0]);
        const minutes = Number(res[1]);
        totalminutes = (hours * 60) + minutes;
      }
      compareString = 'timeofday ' + locOperand + ' ' + totalminutes;
    } else if (timeBlock.attr('type') === 'logic_sunrisesunset') {
      const valueA = $(timeBlock).children('field[name=\'SunriseSunset\']')[0];
      compareString = 'timeofday ' + locOperand + ' @' + $(valueA).text();
    } else if (timeBlock.attr('type').indexOf('uservariables') >= 0) {
      const fieldA = $(timeBlock).find('field[name=\'Variable\']')[0];
      const valueA = 'variable[' + $(fieldA).text() + ']';
      compareString = 'timeofday ' + locOperand + ' tonumber(string.sub(' + valueA + ',1,2))*60+tonumber(string.sub(' + valueA + ',4,5))';
    }
    return compareString;
  }

  private static parseLogicWeekday(thisBlock: any): string {
    const locOperand = BlocklyXmlParserService.opSymbol($($(thisBlock).children('field:first')).text());
    const valueA = $(thisBlock).children('field[name=\'Weekday\']')[0];
    const compareString = 'weekday ' + locOperand + ' ' + $(valueA).text();
    return compareString;
  }

  private static parseSecurityStatus(thisBlock: any): string {
    const locOperand = BlocklyXmlParserService.opSymbol($($(thisBlock).children('field:first')).text());
    const valueA = $(thisBlock).children('field[name=\'Status\']')[0];
    const compareString = 'securitystatus ' + locOperand + ' ' + $(valueA).text();
    return compareString;
  }

  private static parseValueBlock(thisBlock: any, locOperand, Sequence): string {
    const firstBlock = $(thisBlock).children('block:first');
    if (firstBlock.attr('type') === 'logic_compare') {
      const conditionstring = BlocklyXmlParserService.parseLogicCompare(firstBlock);
      return conditionstring;
    } else if (firstBlock.attr('type') === 'logic_weekday') {
      const conditionstring = BlocklyXmlParserService.parseLogicWeekday(firstBlock);
      return conditionstring;
    } else if (firstBlock.attr('type') === 'logic_timeofday') {
      const conditionstring = BlocklyXmlParserService.parseLogicTimeOfDay(firstBlock);
      return conditionstring;
    } else if (firstBlock.attr('type') === 'logic_operation') {
      const conditionstring = BlocklyXmlParserService.parseLogicOperation(firstBlock);
      return conditionstring;
    } else if (firstBlock.attr('type') === 'security_status') {
      const conditionstring = BlocklyXmlParserService.parseSecurityStatus(firstBlock);
      return conditionstring;
    }
  }

  private static parseLogicOperation(thisBlock: any): string {
    const locOperand = ' ' + $($(thisBlock).children('field:first')).text().toLowerCase() + ' ';
    const valueA = $(thisBlock).children('value[name=\'A\']')[0];
    const valueB = $(thisBlock).children('value[name=\'B\']')[0];
    const conditionA = BlocklyXmlParserService.parseValueBlock(valueA, locOperand, 'A');
    const conditionB = BlocklyXmlParserService.parseValueBlock(valueB, locOperand, 'B');
    const conditionstring = '(' + conditionA + ' ' + locOperand + ' ' + conditionB + ')';
    return conditionstring;
  }

  private parseXmlBlocks(xml: any, pairId: number): [Array<string>, Array<string>] {
    let boolString = '';

    const ifBlock = $($(xml).find('value[name=\'IF' + pairId + '\']')[0]).children('block:first');

    if (ifBlock.attr('type') === 'logic_compare') {
      // just the one compare, easy
      const compareString = BlocklyXmlParserService.parseLogicCompare(ifBlock);
      boolString += compareString;
    } else if (ifBlock.attr('type') === 'logic_operation') {
      // nested logic operation, drill down
      const compareString = BlocklyXmlParserService.parseLogicOperation(ifBlock);
      boolString += compareString;
    } else if (ifBlock.attr('type') === 'logic_timeofday') {
      // nested logic operation, drill down
      const compareString = BlocklyXmlParserService.parseLogicTimeOfDay(ifBlock);
      boolString += compareString;
    } else if (ifBlock.attr('type') === 'logic_weekday') {
      // nested logic operation, drill down
      const compareString = BlocklyXmlParserService.parseLogicWeekday(ifBlock);
      boolString += compareString;
    } else if (ifBlock.attr('type') === 'security_status') {
      // nested logic operation, drill down
      const compareString = BlocklyXmlParserService.parseSecurityStatus(ifBlock);
      boolString += compareString;
    }
    const setArray: Array<string> = [];
    const doBlock = $($(xml).find('statement[name=\'DO' + pairId + '\']')[0]);
    $(doBlock).find('block').each(function () {
      if (typeof $(this).attr('disabled') !== 'undefined') {
        return;
      }
      if ($(this).attr('type') === 'logic_set') {
        const valueA = $(this).find('value[name=\'A\']')[0];
        const fieldA = $(valueA).find('field')[0];
        const blockA = $(valueA).children('block:first');
        if (blockA.attr('type').indexOf('uservariables') >= 0) {
          let setString = 'commandArray[Variable:' + $(fieldA).text() + ']';
          const valueB = $(this).find('value[name=\'B\']')[0];
          const fieldB = $(valueB).find('field')[0];
          const blockB = $(valueB).children('block:first');
          const variableTypeB = $(valueB).children('block:first').attr('type');
          const dtext = BlocklyXmlParserService.GetValueText(valueB, variableTypeB);
          setString += '=' + dtext + '';
          setArray.push(setString);
        } else if (blockA.attr('type').indexOf('textvariables') >= 0) {
          let setString = 'commandArray[Text:' + $(fieldA).text() + ']';
          const valueB = $(this).find('value[name=\'B\']')[0];
          const fieldB = $(valueB).find('field')[0];
          const blockB = $(valueB).children('block:first');
          const variableTypeB = $(valueB).children('block:first').attr('type');
          const dtext = BlocklyXmlParserService.GetValueText(valueB, variableTypeB);
          setString += '=' + dtext + '';
          setArray.push(setString);
        } else {
          let setString = 'commandArray[' + $(fieldA).text() + ']';
          const valueB = $(this).find('value[name=\'B\']')[0];
          const fieldB = $(valueB).find('field')[0];
          const blockB = $(valueB).children('block:first');
          if ((blockB.attr('type') === 'logic_states') && ($(fieldB).attr('name') === 'State')) {
            setString += '="' + $(fieldB).text() + '"';
            setArray.push(setString);
          } else if ((blockB.attr('type') === 'logic_setlevel') && ($(fieldB).attr('name') === 'NUM')) {
            setString += '="Set Level ' + $(fieldB).text() + '"';
            setArray.push(setString);
          } else {
            // not handled
            // alert('A Type: ' + blockA.attr("type") + ', B Type: ' + blockB.attr("type") + ', FieldB: ' + $(fieldB).attr("name"));
          }
          // else if ((blockB.attr("type")=="math_number") && ($(fieldB).attr("name") == "NUM")) {
          // 	setString += '="'+$(fieldB).text()+'"';
          // 	setArray.push(setString);
          // }
        }
      } else if ($(this).attr('type') === 'logic_setafter') {
        const valueA = $(this).find('value[name=\'A\']')[0];
        const fieldA = $(valueA).find('field')[0];
        const valueC = $(this).find('value[name=\'C\']')[0];
        const fieldC = $(valueC).find('field')[0];
        const blockA = $(valueA).children('block:first');
        let setString = 'commandArray[' + $(fieldA).text() + ']';
        let valueB = $(this).find('value[name=\'B\']')[0];
        let fieldB = $(valueB).find('field')[0];
        let blockB = $(valueB).children('block:first');

        const blockBType = blockB.attr('type');
        const fieldBName = $(fieldB).attr('name');
        if ((blockBType === 'logic_states') && (fieldBName === 'State')) {
          setString += '="' + $(fieldB).text() + ' AFTER ' + $(fieldC).text() + '"';
          setArray.push(setString);
        } else if ((blockBType === 'logic_setlevel') && (fieldBName === 'NUM')) {
          setString += '="Set Level ' + $(fieldB).text() + ' AFTER ' + $(fieldC).text() + '"';
          setArray.push(setString);
        } else if ((blockBType === 'math_number') && (fieldBName === 'NUM')) {
          if (blockA.attr('type').indexOf('uservariables') >= 0) {
            setString = 'commandArray[Variable:' + $(fieldA).text() + ']';
            valueB = $(this).find('value[name=\'B\']')[0];
            fieldB = $(valueB).find('field')[0];
            blockB = $(valueB).children('block:first');
            setString += '="' + $(fieldB).text() + '';
            setString += ' AFTER ' + $(fieldC).text() + '"';
            setArray.push(setString);
          }
        } else if ((blockBType === 'text') && (fieldBName === 'TEXT')) {
          if (blockA.attr('type').indexOf('uservariables') >= 0) {
            setString = 'commandArray[Variable:' + $(fieldA).text() + ']';
            valueB = $(this).find('value[name=\'B\']')[0];
            fieldB = $(valueB).find('field')[0];
            blockB = $(valueB).children('block:first');
            setString += '="' + $(fieldB).text() + '"';
            setString += ' AFTER ' + $(fieldC).text() + '"';
            setArray.push(setString);
          } else if (blockA.attr('type').indexOf('textvariables') >= 0) {
            setString = 'commandArray[Text:' + $(fieldA).text() + ']';
            valueB = $(this).find('value[name=\'B\']')[0];
            fieldB = $(valueB).find('field')[0];
            blockB = $(valueB).children('block:first');
            setString += '="' + $(fieldB).text() + '"';
            setString += ' AFTER ' + $(fieldC).text() + '"';
            setArray.push(setString);
          }
        }
      } else if ($(this).attr('type') === 'logic_setdelayed') {
        const valueA = $(this).find('value[name=\'A\']')[0];
        const fieldA = $(valueA).find('field')[0];
        const valueC = $(this).find('value[name=\'C\']')[0];
        const fieldC = $(valueC).find('field')[0];
        const blockA = $(valueA).children('block:first');
        let setString = 'commandArray[' + $(fieldA).text() + ']';
        const valueB = $(this).find('value[name=\'B\']')[0];
        const fieldB = $(valueB).find('field')[0];
        const blockB = $(valueB).children('block:first');
        if ((blockB.attr('type') === 'logic_states') && ($(fieldB).attr('name') === 'State')) {
          setString += '="' + $(fieldB).text() + ' FOR ' + $(fieldC).text() + '"';
          setArray.push(setString);
        } else if ((blockB.attr('type') === 'logic_setlevel') && ($(fieldB).attr('name') === 'NUM')) {
          setString += '="Set Level ' + $(fieldB).text() + ' FOR ' + $(fieldC).text() + '"';
          setArray.push(setString);
        }

      } else if ($(this).attr('type') === 'logic_setrandom') {
        const valueA = $(this).find('value[name=\'A\']')[0];
        const fieldA = $(valueA).find('field')[0];
        const valueB = $(this).find('value[name=\'B\']')[0];
        const fieldB = $(valueB).find('field')[0];
        const valueC = $(this).find('value[name=\'C\']')[0];
        const fieldC = $(valueC).find('field')[0];
        const blockA = $(valueA).children('block:first');
        let setString = 'commandArray[' + $(fieldA).text() + ']';
        const blockB = $(valueB).children('block:first');
        if ((blockB.attr('type') === 'logic_states') && ($(fieldB).attr('name') === 'State')) {
          setString += '="' + $(fieldB).text() + ' RANDOM ' + $(fieldC).text() + '"';
          setArray.push(setString);
        } else if ((blockB.attr('type') === 'logic_setlevel') && ($(fieldB).attr('name') === 'NUM')) {
          setString += '="Set Level ' + $(fieldB).text() + ' RANDOM ' + $(fieldC).text() + '"';
          setArray.push(setString);
        }
      } else if ($(this).attr('type') === 'send_notification') {
        const subjectBlock = $(this).find('value[name=\'notificationTextSubject\']')[0];
        const bodyBlock = $(this).find('value[name=\'notificationTextBody\']')[0];
        const notificationBlock = $(this).children('field[name=\'notificationPriority\']')[0];
        const soundBlock = $(this).children('field[name=\'notificationSound\']')[0];
        const subsystemBlock = $(this).children('field[name=\'notificationSubsystem\']')[0];
        const sFieldText = $(subjectBlock).find('field[name=\'TEXT\']')[0];

        const sTT = BlocklyXmlParserService.GetValueText(subjectBlock, $(subjectBlock).children('block:first').attr('type')).replace(/\,/g, ' ');
        const bTT = BlocklyXmlParserService.GetValueText(bodyBlock, $(bodyBlock).children('block:first').attr('type')).replace(/\,/g, ' ');

        const pTT = $(notificationBlock).text();
        const aTT = $(soundBlock).text();
        const subTT = $(subsystemBlock).text();
        // message separator here cannot be # like in scripts, changed to $..
        // also removed commas as we need to separate commandArray later.
        const setString = 'commandArray["SendNotification"]="' + sTT + '$' + bTT + '$' + pTT + '$' + aTT + '$' + subTT + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'send_email') {
        const subjectBlock = $(this).children('field[name=\'TextSubject\']')[0];
        const bodyBlock = $(this).children('field[name=\'TextBody\']')[0];
        const toBlock = $(this).children('field[name=\'TextTo\']')[0];
        const sSubject = $(subjectBlock).text().replace(/\,/g, ' ');
        const sBody = $(bodyBlock).text().replace(/\,/g, ' ');
        const sTo = $(toBlock).text();
        // message separator here cannot be # like in scripts, changed to $..
        // also removed commas as we need to separate commandArray later.
        const setString = 'commandArray["SendEmail"]="' + sSubject + '$' + sBody + '$' + sTo + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'send_sms') {
        const subjectBlock = $(this).children('field[name=\'TextSubject\']')[0];
        const sSubject = $(subjectBlock).text().replace(/\,/g, ' ');
        // message separator here cannot be # like in scripts, changed to $..
        // also removed commas as we need to separate commandArray later.
        const setString = 'commandArray["SendSMS"]="' + sSubject + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'trigger_ifttt') {
        const idBlock = $(this).children('field[name=\'EventID\']')[0];
        const value1Block = $(this).children('field[name=\'TextValue1\']')[0];
        const value2Block = $(this).children('field[name=\'TextValue2\']')[0];
        const value3Block = $(this).children('field[name=\'TextValue3\']')[0];

        const sID = $(idBlock).text();
        const sValue1 = $(value1Block).text().replace(/\,/g, ' ');
        const sValue2 = $(value2Block).text().replace(/\,/g, ' ');
        const sValue3 = $(value3Block).text().replace(/\,/g, ' ');
        // message separator here cannot be # like in scripts, changed to $..
        // also removed commas as we need to separate commandArray later.
        const setString = 'commandArray["TriggerIFTTT"]="' + sID + '$' + sValue1 + '$' + sValue2 + '$' + sValue3 + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'start_script') {
        const pathBlock = $(this).children('field[name=\'TextPath\']')[0];
        const sPath = $(pathBlock).text().replace(/\,/g, ' ');

        const paramBlock = $(this).children('field[name=\'TextParam\']')[0];
        const sParam = $(paramBlock).text().replace(/\,/g, ' ');

        // message separator here cannot be # like in scripts, changed to $..
        // also removed commas as we need to separate commandArray later.
        const setString = 'commandArray["StartScript"]="' + sPath + '$' + sParam + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'open_url') {
        const urlBlock = $(this).find('value[name=\'urlToOpen\']')[0];
        const urlText = $(urlBlock).find('field[name=\'TEXT\']')[0];
        const setString = 'commandArray["OpenURL"]="' + $(urlText).text() + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'open_url_after') {
        const urlBlock = $(this).find('value[name=\'urlToOpen\']')[0];
        const urlText = $(urlBlock).find('field[name=\'TEXT\']')[0];
        const urlAfter = $(this).children('field[name=\'urlAfter\']')[0];
        let setString = 'commandArray["OpenURL"]="' + $(urlText).text();
        setString += ' AFTER ' + $(urlAfter).text() + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'writetolog') {
        const logBlock = $(this).find('value[name=\'writeToLog\']')[0];
        const blockInfo = $(logBlock).children('block:first');
        let setString = '';
        if (blockInfo.attr('type') === 'text') {
          const logText = $(blockInfo).find('field[name=\'TEXT\']')[0];
          setString = 'commandArray["WriteToLogText"]="' + $(logText).text() + '"';
          setArray.push(setString);
        } else if (blockInfo.attr('type').indexOf('uservariables') >= 0) {
          const userVar = $(blockInfo).find('field[name=\'Variable\']')[0];
          setString = 'commandArray["WriteToLogUserVariable"]="' + $(userVar).text() + '"';
          setArray.push(setString);
        } else if (blockInfo.attr('type').indexOf('switchvariables') >= 0) {
          const switchVar = $(blockInfo).find('field')[0];
          setString = 'commandArray["WriteToLogSwitch"]="' + $(switchVar).text() + '"';
          setArray.push(setString);
        } else if (blockInfo.attr('type').indexOf('variables') >= 0) {
          const deviceVar = $(blockInfo).find('field')[0];
          setString = 'commandArray["WriteToLogDeviceVariable"]="' + $(deviceVar).text() + '"';
          setArray.push(setString);
        }
      } else if ($(this).attr('type') === 'groupvariables') {
        const fieldA = $(this).find('field[name=\'Group\']')[0];
        const fieldB = $(this).find('field[name=\'Status\']')[0];
        let setString = 'commandArray[Group:' + $(fieldA).text() + ']';
        setString += '="' + $(fieldB).text() + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'scenevariables') {
        const fieldA = $(this).find('field[name=\'Scene\']')[0];
        const fieldB = $(this).find('field[name=\'Status\']')[0];
        let setString = 'commandArray[Scene:' + $(fieldA).text() + ']';
        setString += '="' + $(fieldB).text() + '"';
        setArray.push(setString);
      } else if ($(this).attr('type') === 'cameravariables') {
        const fieldA = $(this).find('field[name=\'Camera\']')[0];
        const fieldB = $(this).find('field[name=\'Subject\']')[0];
        const fieldC = $(this).find('field[name=\'NUM\']')[0];
        let setString = 'commandArray[SendCamera:' + $(fieldA).text() + ']';
        setString += '="' + $(fieldB).text() + '" AFTER ' + $(fieldC).text().replace(/\,/g, ' ');
        setArray.push(setString);
      } else if ($(this).attr('type') === 'setpointvariables') {
        const fieldA = $(this).find('field[name=\'SetPoint\']')[0];
        const fieldB = $(this).find('field[name=\'NUM\']')[0];
        let setString = 'commandArray[SetSetpoint:' + $(fieldA).text() + ']';
        setString += '="' + $(fieldB).text() + '"';
        setArray.push(setString);
      }
    });
    const conditionArray: Array<string> = [];
    conditionArray.push(boolString);
    return [conditionArray, setArray];
  }

  public parseXml(xml: any): ParsedXml { // xml = DOM element
    if ($(xml).children().length !== 1) {
      throw new Error('Please make sure there is only a single block structure');
    }

    const firstBlockType = $(xml).find('block').first().attr('type');

    if (firstBlockType.indexOf('domoticzcontrols_if') === -1) {
      throw new Error('Please start with a control block');
    }

    const json: ParsedXml = {
      eventlogic: []
    };

    if ($(xml).find('block').first().attr('disabled') !== undefined) {
      alert($(xml).find('block').first().attr('disabled'));
      alert('disabled!!');
      return json;
    }


    let elseIfCount = 0;
    if (firstBlockType === 'domoticzcontrols_ifelseif') {
      const elseIfString = $(xml).find('mutation:first').attr('elseif');
      elseIfCount = Number(elseIfString);
    }
    elseIfCount++;


    for (let i = 0; i < elseIfCount; i++) {
      const conditionActionPair = this.parseXmlBlocks(xml, i);
      const oneevent: EventLogicItem = {} as EventLogicItem;
      oneevent.conditions = conditionActionPair[0].toString();
      oneevent.actions = conditionActionPair[1].toString();
      if (oneevent.actions.length > 0) {
        json.eventlogic.push(oneevent);
      }
    }

    return json;
  }

}

export interface ParsedXml {
  eventlogic: Array<EventLogicItem>;
}

export interface EventLogicItem {
  conditions: string;
  actions: string;
}
