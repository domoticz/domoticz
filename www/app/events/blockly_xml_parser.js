define(function () {
    return {
        parseXml: parseXml
    };

    function parseXml(xml) {

        if ($(xml).children().length !== 1) {
            throw new Error('Please make sure there is only a single block structure');
        }

        var firstBlockType = $(xml).find('block').first().attr('type');

        if (firstBlockType.indexOf('domoticzcontrols_if') == -1) {
            throw new Error('Please start with a control block');
        }

        var json = {
            eventlogic: []
        };
        
		if ($(xml).find('block').first().attr('disabled') !== undefined) {
			alert($(xml).find('block').first().attr('disabled'));
			alert("disabled!!");
			return json;
		}
        

        var elseIfCount = 0;
        if (firstBlockType == 'domoticzcontrols_ifelseif') {
            var elseIfString = $(xml).find('mutation:first').attr('elseif');
            elseIfCount = parseInt(elseIfString);
        }
        elseIfCount++;


        for (var i = 0; i < elseIfCount; i++) {
            var conditionActionPair = parseXmlBlocks(xml, i);
            var oneevent = {};
            oneevent.conditions = conditionActionPair[0].toString();
            oneevent.actions = conditionActionPair[1].toString();
            if (oneevent.actions.length>0) {
				json.eventlogic.push(oneevent);
			}
        }

        return json;
    }

    function parseXmlBlocks(xml, pairId) {
        var boolString = '';

        function GetValueText(value, variableType) {
            if (variableType.indexOf('switchvariables') >= 0) {
                var fieldA = $(value).find('field')[0];
                return 'device[' + $(fieldA).text() + ']';
            }
            else if (variableType.indexOf('uservariables') >= 0) {
                var fieldA = $(value).find('field')[0];
                return 'variable[' + $(fieldA).text() + ']';
            }
            else if (variableType == 'temperaturevariables') {
                var fieldA = $(value).find('field')[0];
                return 'temperaturedevice[' + $(fieldA).text() + ']';
            }
            else if (variableType == 'humidityvariables') {
                var fieldA = $(value).find('field')[0];
                return 'humiditydevice[' + $(fieldA).text() + ']';
            }
            else if (variableType == 'dewpointvariables') {
                var fieldA = $(value).find('field')[0];
                return 'dewpointdevice[' + $(fieldA).text() + ']';
            }
            else if (variableType == 'barometervariables') {
                var fieldA = $(value).find('field')[0];
                return 'barometerdevice[' + $(fieldA).text() + ']';
            }
            else if (
                (variableType.indexOf('utilityvariables') >= 0) ||
                (variableType.indexOf('textvariables') >= 0)
            ) {
                var fieldA = $(value).find('field')[0];
                return 'utilitydevice[' + $(fieldA).text() + ']';
            }
            else if (variableType.indexOf('weathervariables') >= 0) {
                var fieldA = $(value).find('field')[0];
                return 'weatherdevice[' + $(fieldA).text() + ']';
            }
            else if (variableType.indexOf('zwavealarms') >= 0) {
                var fieldA = $(value).find('field')[0];
                return 'zwavealarms[' + $(fieldA).text() + ']';
            }
            else {
                var fieldB = $(value).find('field')[0];
                if ($(fieldB).attr('name') == 'State') {
                    return '"' + $(fieldB).text() + '"';
                }
                else if ($(fieldB).attr('name') == 'TEXT') {
                    return '"' + $(fieldB).text() + '"';
                }
                else if ($(fieldB).attr('name') == 'NUM') {
                    return $(fieldB).text();
                }
                return 'unknown comparevariable ' + variableType;
            }
        }

        function parseLogicCompare(thisBlock) {
            var locOperand = opSymbol($($(thisBlock).children('field:first')).text());
            var valueA = $(thisBlock).children('value[name=\'A\']')[0];
            var variableTypeA = $(valueA).children('block:first').attr('type');
            var valueB = $(thisBlock).children('value[name=\'B\']')[0];
            var variableTypeB = $(valueB).children('block:first').attr('type');
            var varTextA = GetValueText(valueA, variableTypeA);
            var varTextB = GetValueText(valueB, variableTypeB);

            var compareString = varTextA;
            compareString += locOperand;
            compareString += varTextB;
            return compareString;
        }

        function parseLogicTimeOfDay(thisBlock) {
            var compareString = '';
            var locOperand = opSymbol($($(thisBlock).children('field:first')).text());
            var valueTime = $(thisBlock).children('value[name=\'Time\']')[0];
            var timeBlock = $(valueTime).children('block:first');
            if (timeBlock.attr('type') == 'logic_timevalue') {
                var valueA = $(timeBlock).children('field[name=\'TEXT\']')[0];
                var hours = parseInt($(valueA).text().substr(0, 2));
                var minutes = parseInt($(valueA).text().substr(3, 2));
                var totalminutes = (hours * 60) + minutes;
                compareString = 'timeofday ' + locOperand + ' ' + totalminutes;
            }
            else if (timeBlock.attr('type') == 'logic_sunrisesunset') {
                var valueA = $(timeBlock).children('field[name=\'SunriseSunset\']')[0];
                compareString = 'timeofday ' + locOperand + ' @' + $(valueA).text();
            }
            else if (timeBlock.attr('type').indexOf('uservariables') >= 0) {
                var fieldA = $(timeBlock).find('field[name=\'Variable\']')[0];
                var valueA = 'variable[' + $(fieldA).text() + ']';
                compareString = 'timeofday ' + locOperand + ' tonumber(string.sub(' + valueA + ',1,2))*60+tonumber(string.sub(' + valueA + ',4,5))';
            }
            return compareString;
        }

        function parseLogicWeekday(thisBlock) {
            var locOperand = opSymbol($($(thisBlock).children('field:first')).text());
            var valueA = $(thisBlock).children('field[name=\'Weekday\']')[0];
            var compareString = 'weekday ' + locOperand + ' ' + $(valueA).text();
            return compareString;
        }

        function parseSecurityStatus(thisBlock) {
            var locOperand = opSymbol($($(thisBlock).children('field:first')).text());
            var valueA = $(thisBlock).children('field[name=\'Status\']')[0];
            var compareString = 'securitystatus ' + locOperand + ' ' + $(valueA).text();
            return compareString;
        }

        function parseValueBlock(thisBlock, locOperand, Sequence) {
            var firstBlock = $(thisBlock).children('block:first');
            if (firstBlock.attr('type') == 'logic_compare') {
                var conditionstring = parseLogicCompare(firstBlock);
                return conditionstring;
            }
            else if (firstBlock.attr('type') == 'logic_weekday') {
                var conditionstring = parseLogicWeekday(firstBlock);
                return conditionstring;
            }
            else if (firstBlock.attr('type') == 'logic_timeofday') {
                var conditionstring = parseLogicTimeOfDay(firstBlock);
                return conditionstring;
            }
            else if (firstBlock.attr('type') == 'logic_operation') {
                var conditionstring = parseLogicOperation(firstBlock);
                return conditionstring;
            }
            else if (firstBlock.attr('type') == 'security_status') {
                var conditionstring = parseSecurityStatus(firstBlock);
                return conditionstring;
            }
        }

        function parseLogicOperation(thisBlock) {
            var locOperand = ' ' + $($(thisBlock).children('field:first')).text().toLowerCase() + ' ';
            var valueA = $(thisBlock).children('value[name=\'A\']')[0];
            var valueB = $(thisBlock).children('value[name=\'B\']')[0];
            var conditionA = parseValueBlock(valueA, locOperand, 'A');
            var conditionB = parseValueBlock(valueB, locOperand, 'B');
            var conditionstring = '(' + conditionA + ' ' + locOperand + ' ' + conditionB + ')';
            return conditionstring;
        }

        var ifBlock = $($(xml).find('value[name=\'IF' + pairId + '\']')[0]).children('block:first');

        if (ifBlock.attr('type') == 'logic_compare') {
            // just the one compare, easy
            var compareString = parseLogicCompare(ifBlock);
            boolString += compareString;
        }
        else if (ifBlock.attr('type') == 'logic_operation') {
            // nested logic operation, drill down
            var compareString = parseLogicOperation(ifBlock);
            boolString += compareString;

        }
        else if (ifBlock.attr('type') == 'logic_timeofday') {
            // nested logic operation, drill down
            var compareString = parseLogicTimeOfDay(ifBlock);
            boolString += compareString;
        }
        else if (ifBlock.attr('type') == 'logic_weekday') {
            // nested logic operation, drill down
            var compareString = parseLogicWeekday(ifBlock);
            boolString += compareString;
        }
        else if (ifBlock.attr('type') == 'security_status') {
            // nested logic operation, drill down
            var compareString = parseSecurityStatus(ifBlock);
            boolString += compareString;
        }
        var setArray = [];
        var doBlock = $($(xml).find('statement[name=\'DO' + pairId + '\']')[0]);
        $(doBlock).find('block').each(function () {
			if (typeof $(this).attr('disabled') != 'undefined') {
				return;
			}
            if ($(this).attr('type') == 'logic_set') {
                var valueA = $(this).find('value[name=\'A\']')[0];
                var fieldA = $(valueA).find('field')[0];
                var blockA = $(valueA).children('block:first');
                if (blockA.attr('type').indexOf('uservariables') >= 0) {
                    var setString = 'commandArray[Variable:' + $(fieldA).text() + ']';
                    var valueB = $(this).find('value[name=\'B\']')[0];
                    var fieldB = $(valueB).find('field')[0];
                    var blockB = $(valueB).children('block:first');
                    var variableTypeB = $(valueB).children('block:first').attr('type');
                    var dtext = GetValueText(valueB, variableTypeB);
                    setString += '=' + dtext + '';
                    setArray.push(setString);
                }
                else if (blockA.attr('type').indexOf('textvariables') >= 0) {
                    var setString = 'commandArray[Text:' + $(fieldA).text() + ']';
                    var valueB = $(this).find('value[name=\'B\']')[0];
                    var fieldB = $(valueB).find('field')[0];
                    var blockB = $(valueB).children('block:first');
                    var variableTypeB = $(valueB).children('block:first').attr('type');
                    var dtext = GetValueText(valueB, variableTypeB);
                    setString += '=' + dtext + '';
                    setArray.push(setString);
                }
                else {
                    var setString = 'commandArray[' + $(fieldA).text() + ']';
                    var valueB = $(this).find('value[name=\'B\']')[0];
                    var fieldB = $(valueB).find('field')[0];
                    var blockB = $(valueB).children('block:first');
                    if ((blockB.attr('type') == 'logic_states') && ($(fieldB).attr('name') == 'State')) {
                        setString += '="' + $(fieldB).text() + '"';
                        setArray.push(setString);
                    }
                    else if ((blockB.attr('type') == 'logic_setlevel') && ($(fieldB).attr('name') == 'NUM')) {
                        setString += '="Set Level ' + $(fieldB).text() + '"';
                        setArray.push(setString);
                    }
                    else {
                        //not handled
                        //alert('A Type: ' + blockA.attr("type") + ', B Type: ' + blockB.attr("type") + ', FieldB: ' + $(fieldB).attr("name"));
                    }
                    //else if ((blockB.attr("type")=="math_number") && ($(fieldB).attr("name") == "NUM")) {
                    //	setString += '="'+$(fieldB).text()+'"';
                    //	setArray.push(setString);
                    //}
                }
            }
            else if ($(this).attr('type') == 'logic_setafter') {
                var valueA = $(this).find('value[name=\'A\']')[0];
                var fieldA = $(valueA).find('field')[0];
                var valueC = $(this).find('value[name=\'C\']')[0];
                var fieldC = $(valueC).find('field')[0];
                var blockA = $(valueA).children('block:first');
                var setString = 'commandArray[' + $(fieldA).text() + ']';
                var valueB = $(this).find('value[name=\'B\']')[0];
                var fieldB = $(valueB).find('field')[0];
                var blockB = $(valueB).children('block:first');

                var blockBType = blockB.attr('type');
                var fieldBName = $(fieldB).attr('name');
                if ((blockBType == 'logic_states') && (fieldBName == 'State')) {
                    setString += '="' + $(fieldB).text() + ' AFTER ' + $(fieldC).text() + '"';
                    setArray.push(setString);
                }
                else if ((blockBType == 'logic_setlevel') && (fieldBName == 'NUM')) {
                    setString += '="Set Level ' + $(fieldB).text() + ' AFTER ' + $(fieldC).text() + '"';
                    setArray.push(setString);
                }
                else if ((blockBType == 'math_number') && (fieldBName == 'NUM')) {
                    if (blockA.attr('type').indexOf('uservariables') >= 0) {
                        var setString = 'commandArray[Variable:' + $(fieldA).text() + ']';
                        var valueB = $(this).find('value[name=\'B\']')[0];
                        var fieldB = $(valueB).find('field')[0];
                        var blockB = $(valueB).children('block:first');
                        setString += '="' + $(fieldB).text() + '';
                        setString += ' AFTER ' + $(fieldC).text() + '"';
                        setArray.push(setString);
                    }
                }
                else if ((blockBType == 'text') && (fieldBName == 'TEXT')) {
                    if (blockA.attr('type').indexOf('uservariables') >= 0) {
                        var setString = 'commandArray[Variable:' + $(fieldA).text() + ']';
                        var valueB = $(this).find('value[name=\'B\']')[0];
                        var fieldB = $(valueB).find('field')[0];
                        var blockB = $(valueB).children('block:first');
                        setString += '="' + $(fieldB).text() + '"';
                        setString += ' AFTER ' + $(fieldC).text() + '"';
                        setArray.push(setString);
                    }
                    else if (blockA.attr('type').indexOf('textvariables') >= 0) {
                        var setString = 'commandArray[Text:' + $(fieldA).text() + ']';
                        var valueB = $(this).find('value[name=\'B\']')[0];
                        var fieldB = $(valueB).find('field')[0];
                        var blockB = $(valueB).children('block:first');
                        setString += '="' + $(fieldB).text() + '"';
                        setString += ' AFTER ' + $(fieldC).text() + '"';
                        setArray.push(setString);
                    }
                }
            }
            else if ($(this).attr('type') == 'logic_setdelayed') {
                var valueA = $(this).find('value[name=\'A\']')[0];
                var fieldA = $(valueA).find('field')[0];
                var valueC = $(this).find('value[name=\'C\']')[0];
                var fieldC = $(valueC).find('field')[0];
                var blockA = $(valueA).children('block:first');
                var setString = 'commandArray[' + $(fieldA).text() + ']';
                var valueB = $(this).find('value[name=\'B\']')[0];
                var fieldB = $(valueB).find('field')[0];
                var blockB = $(valueB).children('block:first');
                if ((blockB.attr('type') == 'logic_states') && ($(fieldB).attr('name') == 'State')) {
                    setString += '="' + $(fieldB).text() + ' FOR ' + $(fieldC).text() + '"';
                    setArray.push(setString);
                }
                else if ((blockB.attr('type') == 'logic_setlevel') && ($(fieldB).attr('name') == 'NUM')) {
                    setString += '="Set Level ' + $(fieldB).text() + ' FOR ' + $(fieldC).text() + '"';
                    setArray.push(setString);
                }

            }
            else if ($(this).attr('type') == 'logic_setrandom') {
                var valueA = $(this).find('value[name=\'A\']')[0];
                var fieldA = $(valueA).find('field')[0];
                var valueB = $(this).find('value[name=\'B\']')[0];
                var fieldB = $(valueB).find('field')[0];
                var valueC = $(this).find('value[name=\'C\']')[0];
                var fieldC = $(valueC).find('field')[0];
                var blockA = $(valueA).children('block:first');
                var setString = 'commandArray[' + $(fieldA).text() + ']';
                var blockB = $(valueB).children('block:first');
                if ((blockB.attr('type') == 'logic_states') && ($(fieldB).attr('name') == 'State')) {
                    setString += '="' + $(fieldB).text() + ' RANDOM ' + $(fieldC).text() + '"';
                    setArray.push(setString);
                }
                else if ((blockB.attr('type') == 'logic_setlevel') && ($(fieldB).attr('name') == 'NUM')) {
                    setString += '="Set Level ' + $(fieldB).text() + ' RANDOM ' + $(fieldC).text() + '"';
                    setArray.push(setString);
                }
            }
            else if ($(this).attr('type') == 'send_notification') {
                var subjectBlock = $(this).find('value[name=\'notificationTextSubject\']')[0];
                var bodyBlock = $(this).find('value[name=\'notificationTextBody\']')[0];
                var notificationBlock = $(this).children('field[name=\'notificationPriority\']')[0];
                var soundBlock = $(this).children('field[name=\'notificationSound\']')[0];
                var subsystemBlock = $(this).children('field[name=\'notificationSubsystem\']')[0];
                var sFieldText = $(subjectBlock).find('field[name=\'TEXT\']')[0];

                var sTT = GetValueText(subjectBlock, $(subjectBlock).children('block:first').attr('type')).replace(/\,/g, ' ');
                var bTT = GetValueText(bodyBlock, $(bodyBlock).children('block:first').attr('type')).replace(/\,/g, ' ');

                var pTT = $(notificationBlock).text();
                var aTT = $(soundBlock).text();
                var subTT = $(subsystemBlock).text();
                // message separator here cannot be # like in scripts, changed to $..
                // also removed commas as we need to separate commandArray later.
                var setString = 'commandArray["SendNotification"]="' + sTT + '$' + bTT + '$' + pTT + '$' + aTT + '$' + subTT + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'send_email') {
                var subjectBlock = $(this).children('field[name=\'TextSubject\']')[0];
                var bodyBlock = $(this).children('field[name=\'TextBody\']')[0];
                var toBlock = $(this).children('field[name=\'TextTo\']')[0];
                var sSubject = $(subjectBlock).text().replace(/\,/g, ' ');
                var sBody = $(bodyBlock).text().replace(/\,/g, ' ');
                var sTo = $(toBlock).text();
                // message separator here cannot be # like in scripts, changed to $..
                // also removed commas as we need to separate commandArray later.
                var setString = 'commandArray["SendEmail"]="' + sSubject + '$' + sBody + '$' + sTo + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'send_sms') {
                var subjectBlock = $(this).children('field[name=\'TextSubject\']')[0];
                var sSubject = $(subjectBlock).text().replace(/\,/g, ' ');
                // message separator here cannot be # like in scripts, changed to $..
                // also removed commas as we need to separate commandArray later.
                var setString = 'commandArray["SendSMS"]="' + sSubject + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'trigger_ifttt') {
                var idBlock = $(this).children('field[name=\'EventID\']')[0];
                var value1Block = $(this).children('field[name=\'TextValue1\']')[0];
                var value2Block = $(this).children('field[name=\'TextValue2\']')[0];
                var value3Block = $(this).children('field[name=\'TextValue3\']')[0];

                var sID = $(idBlock).text();
                var sValue1 = $(value1Block).text().replace(/\,/g, ' ');
                var sValue2 = $(value2Block).text().replace(/\,/g, ' ');
                var sValue3 = $(value3Block).text().replace(/\,/g, ' ');
                // message separator here cannot be # like in scripts, changed to $..
                // also removed commas as we need to separate commandArray later.
                var setString = 'commandArray["TriggerIFTTT"]="' + sID + '$' + sValue1 + '$' + sValue2 + '$' + sValue3 + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'start_script') {
                var pathBlock = $(this).children('field[name=\'TextPath\']')[0];
                var sPath = $(pathBlock).text().replace(/\,/g, ' ');

                var paramBlock = $(this).children('field[name=\'TextParam\']')[0];
                var sParam = $(paramBlock).text().replace(/\,/g, ' ');

                // message separator here cannot be # like in scripts, changed to $..
                // also removed commas as we need to separate commandArray later.
                var setString = 'commandArray["StartScript"]="' + sPath + '$' + sParam + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'open_url') {
                var urlBlock = $(this).find('value[name=\'urlToOpen\']')[0];
                var urlText = $(urlBlock).find('field[name=\'TEXT\']')[0];
                var setString = 'commandArray["OpenURL"]="' + $(urlText).text() + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'open_url_after') {
                var urlBlock = $(this).find('value[name=\'urlToOpen\']')[0];
                var urlText = $(urlBlock).find('field[name=\'TEXT\']')[0];
                var urlAfter = $(this).children('field[name=\'urlAfter\']')[0];
                var setString = 'commandArray["OpenURL"]="' + $(urlText).text();
                setString += ' AFTER ' + $(urlAfter).text() + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'writetolog') {
                var logBlock = $(this).find('value[name=\'writeToLog\']')[0];
                var blockInfo = $(logBlock).children('block:first');
                var setString = '';
                if (blockInfo.attr('type') == 'text') {
                    var logText = $(blockInfo).find('field[name=\'TEXT\']')[0];
                    setString = 'commandArray["WriteToLogText"]="' + $(logText).text() + '"';
                    setArray.push(setString);
                }
                else if (blockInfo.attr('type').indexOf('uservariables') >= 0) {
                    var userVar = $(blockInfo).find('field[name=\'Variable\']')[0];
                    setString = 'commandArray["WriteToLogUserVariable"]="' + $(userVar).text() + '"';
                    setArray.push(setString);
                }
                else if (blockInfo.attr('type').indexOf('switchvariables') >= 0) {
                    var switchVar = $(blockInfo).find('field')[0];
                    setString = 'commandArray["WriteToLogSwitch"]="' + $(switchVar).text() + '"';
                    setArray.push(setString);
                }
                else if (blockInfo.attr('type').indexOf('variables') >= 0) {
                    var deviceVar = $(blockInfo).find('field')[0];
                    setString = 'commandArray["WriteToLogDeviceVariable"]="' + $(deviceVar).text() + '"';
                    setArray.push(setString);
                }
            }
            else if ($(this).attr('type') == 'groupvariables') {
                var fieldA = $(this).find('field[name=\'Group\']')[0];
                var fieldB = $(this).find('field[name=\'Status\']')[0];
                var setString = 'commandArray[Group:' + $(fieldA).text() + ']';
                setString += '="' + $(fieldB).text() + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'scenevariables') {
                var fieldA = $(this).find('field[name=\'Scene\']')[0];
                var fieldB = $(this).find('field[name=\'Status\']')[0];
                var setString = 'commandArray[Scene:' + $(fieldA).text() + ']';
                setString += '="' + $(fieldB).text() + '"';
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'cameravariables') {
                var fieldA = $(this).find('field[name=\'Camera\']')[0];
                var fieldB = $(this).find('field[name=\'Subject\']')[0];
                var fieldC = $(this).find('field[name=\'NUM\']')[0];
                var setString = 'commandArray[SendCamera:' + $(fieldA).text() + ']';
                setString += '="' + $(fieldB).text() + '" AFTER ' + $(fieldC).text().replace(/\,/g, ' ');
                setArray.push(setString);
            }
            else if ($(this).attr('type') == 'setpointvariables') {
                var fieldA = $(this).find('field[name=\'SetPoint\']')[0];
                var fieldB = $(this).find('field[name=\'NUM\']')[0];
                var setString = 'commandArray[SetSetpoint:' + $(fieldA).text() + ']';
                setString += '="' + $(fieldB).text() + '"';
                setArray.push(setString);
            }
        });
        var conditionArray = [];
        conditionArray.push(boolString);
        return [conditionArray, setArray];
    }

    function opSymbol(operand) {
        switch(operand)
        {
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
});
