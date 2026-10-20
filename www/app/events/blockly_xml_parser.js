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
            if (!variableType) {
                // Unconnected input, or a value holding only a <shadow> block: read the shadow's
                // default field so a block left with its default input still generates valid Lua.
                var shadow = $(value).children('shadow:first')[0];
                var src = shadow || value;
                var sf = $(src).find('field')[0];
                if (sf) {
                    var sn = $(sf).attr('name');
                    if (sn == 'NUM') return $(sf).text();
                    if (sn == 'TEXT' || sn == 'State') return '"' + $(sf).text() + '"';
                }
                return 'nil';
            }
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

        function resolveValue(value, variableType) {
            if (!variableType) {
                return GetValueText(value, variableType); // shadow-only / unconnected input
            }
            // Resolve a value input to Lua text. Handles nested math_arithmetic blocks.
            if (variableType == 'math_arithmetic') {
                var arithBlock = $(value).children('block:first');
                var opField = $(arithBlock).children('field[name=\'OP\']')[0];
                var opMap = { ADD: '+', MINUS: '-', SUBTRACT: '-', MULTIPLY: '*', DIVIDE: '/', POWER: '^' };
                var sym = opMap[$(opField).text()] || '+';
                var valueA = $(arithBlock).children('value[name=\'A\']')[0];
                var typeA = $(valueA).children('block:first').attr('type');
                var valueB = $(arithBlock).children('value[name=\'B\']')[0];
                var typeB = $(valueB).children('block:first').attr('type');
                return '(' + resolveValue(valueA, typeA) + ' ' + sym + ' ' + resolveValue(valueB, typeB) + ')';
            }
            // ---- v13 standard value blocks (patched 2026-10-01) ----
            // Each returns a Lua *expression*. Domoticz's event interpreter runs
            // under Lua 5.4 with the full math + string libs, so we map directly.
            var _b = $(value).children('block:first');   // the actual <block> inside the <value> wrapper
            function _val(vname) { var v = $(_b).children('value[name=\'' + vname + '\']')[0]; return v ? resolveValue(v, $(v).children('block:first').attr('type')) : ''; }
            function _fld(fname) { var f = $(_b).children('field[name=\'' + fname + '\']')[0]; return f ? $(f).text() : ''; }
            function _mut(attr, dflt) { var m = $(_b).children('mutation:first')[0]; var v = m ? $(m).attr(attr) : undefined; return (v === undefined || v === null) ? dflt : v; }
            function _fldOrMut(fname, attr, dflt) { var v = _fld(fname); if (v) return v; v = _mut(attr, null); return v !== null ? v : dflt; }
            // reverse a Lua array (returns a new array)
            function _rev(a) { return '(function(t) local r={} for i=#t,1,-1 do r[#r+1]=t[i] end return r end)' + '(' + a + ')' }
            function _median(a) { return '(function(t) local s={} for i=1,#t do s[i]=t[i] end table.sort(s) local n=#s if n==0 then return 0 end if n%2==1 then return s[(n+1)/2] end return (s[n/2]+s[n/2+1])/2 end)' + '(' + a + ')' }
            function _stddev(a) { return '(function(t) local n=#t if n==0 then return 0 end local s=0 for i=1,n do s=s+t[i] end local m=s/n local q=0 for i=1,n do q=q+(t[i]-m)^2 end return math.sqrt(q/n) end)' + '(' + a + ')' }

            if (variableType == 'logic_boolean') {
                var bv = _fld('BOOL');
                return bv == 'TRUE' ? 'true' : 'false';
            }
            else if (variableType == 'math_number') {
                return _fld('NUM');
            }
            else if (variableType == 'text') {
                return '"' + _fld('TEXT') + '"';
            }
            else if (variableType == 'math_constant') {
                var c = _fld('CONSTANT');
                if (c == 'E') return 'math.exp(1)';
                if (c == 'GOLDEN_RATIO') return '((1+math.sqrt(5))/2)';
                if (c == 'SQRT2') return 'math.sqrt(2)';
                if (c == 'SQRT1_2') return 'math.sqrt(0.5)';
                if (c == 'INFINITY') return 'math.huge';
                return 'math.pi'; // PI
            }
            else if (variableType == 'math_round') {
                var op = _fld('OP'), num = _val('NUM');
                if (op == 'ROUNDUP')   return 'math.ceil(' + num + ')'
                if (op == 'ROUNDDOWN') return 'math.floor(' + num + ')'
                return '(function(n) return math.floor(n + 0.5) end)' + '(' + num + ')' // ROUND
            }
            else if (variableType == 'math_single') {
                var op = _fld('OP'), num = _val('NUM');
                var singleMap = { ROOT: 'math.sqrt', ABS: 'math.abs', NEG: '-', LN: 'math.log', EXP: 'math.exp' };
                if (op == 'POWTEN' || op == 'POW10') return '(10^(' + num + '))'; // 10^num (v13 renamed POWTEN -> POW10)
                if (op == 'LOG10') return 'math.log(' + num + ',10)'; // Lua 5.4 has no math.log10
                if (singleMap[op]) return singleMap[op] + '(' + num + ')';
                return num;
            }
            else if (variableType == 'math_trig') {
                var op = _fld('OP'), num = _val('NUM');
                // Blockly trig works in degrees; Lua math.* work in radians.
                var trigMap = {
                    SIN: ['math.sin', true], COS: ['math.cos', true], TAN: ['math.tan', true],
                    ASIN: ['math.asin', false], ACOS: ['math.acos', false], ATAN: ['math.atan', false]
                };
                var t = trigMap[op]; if (!t) return num;
                if (t[1]) return t[0] + '(' + num + '*math.pi/180)';
                return '((180/math.pi)*' + t[0] + '(' + num + '))';
            }
            else if (variableType == 'math_atan2') {
                // Blockly trig works in degrees; Lua atan returns radians.
                return '((180/math.pi)*math.atan(' + _val('Y') + ',' + _val('X') + '))'; // Lua 5.4: atan(y,x)
            }
            else if (variableType == 'math_modulo') {
                return '(function(a,b) return a - math.floor(a/b)*b end)' + '(' + _val('DIVIDEND') + ',' + _val('DIVISOR') + ')'
            }
            else if (variableType == 'math_constrain') {
                return '(function(v,lo,hi) return math.min(math.max(v,lo),hi) end)' + '(' + _val('VALUE') + ',' + _val('LOW') + ',' + _val('HIGH') + ')'
            }
            else if (variableType == 'math_random_float') {
                return 'math.random()';
            }
            else if (variableType == 'math_random_int') {
                // Blockly is inclusive of TO; Lua math.random(a,b) is inclusive too.
                return 'math.random(' + _val('FROM') + ',' + _val('TO') + ')';
            }
            else if (variableType == 'math_on_list') {
                var op = _fld('OP'), list = _val('LIST');
                if (op == 'SUM')     return '(function(t) local s=0 for i=1,#t do s=s+t[i] end return s end)' + '(' + list + ')'
                if (op == 'MIN')     return '(function(t) if #t==0 then return nil end local m=t[1] for i=2,#t do if t[i]<m then m=t[i] end end return m end)' + '(' + list + ')'
                if (op == 'MAX')     return '(function(t) if #t==0 then return nil end local m=t[1] for i=2,#t do if t[i]>m then m=t[i] end end return m end)' + '(' + list + ')'
                if (op == 'AVERAGE') return '(function(t) if #t==0 then return 0 end local s=0 for i=1,#t do s=s+t[i] end return s/#t end)' + '(' + list + ')'
                if (op == 'MEDIAN')  return _median(list);
                if (op == 'MODE')    return '(function(t) local c={} local best=0 for i=1,#t do c[t[i]]=(c[t[i]] or 0)+1 if c[t[i]]>best then best=c[t[i]] end end local r={} for k,v in pairs(c) do if v==best then r[#r+1]=k end end if #r==1 then return r[1] end return r end)' + '(' + list + ')'
                if (op == 'STD_DEV') return _stddev(list);
                if (op == 'RANDOM')  return '(function(t) if #t==0 then return nil end return t[math.random(1,#t)] end)' + '(' + list + ')'
                return list;
            }
            else if (variableType == 'lists_create_with') {
                var parts = [];
                $(_b).children('value').each(function () { parts.push(resolveValue(this, $(this).children('block:first').attr('type'))); });
                return '{' + parts.join(',') + '}';
            }
            else if (variableType == 'lists_repeat') {
                var item = _val('ITEM'), n = _val('NUM');
                return '(function(it,n) local r={} for i=1,n do r[i]=it end return r end)' + '(' + item + ',' + n + ')'
            }
            else if (variableType == 'lists_length') {
                return '#' + _val('VALUE');
            }
            else if (variableType == 'lists_isEmpty') {
                return '(#' + _val('VALUE') + ')==0';
            }
            else if (variableType == 'lists_indexOf') {
                var list = _val('VALUE'), find = _val('FIND'), end_ = _fld('END');
                if (end_ == 'LAST') return '(function(t,x) for i=#t,1,-1 do if t[i]==x then return i end end return 0 end)' + '(' + list + ',' + find + ')'
                return '(function(t,x) for i=1,#t do if t[i]==x then return i end end return 0 end)' + '(' + list + ',' + find + ')' // FIRST
            }
            else if (variableType == 'lists_getIndex') {
                var list = _val('VALUE'), mode = _fld('MODE'), where = _fld('WHERE');
                var atEl = $(_b).children('value[name=\'AT\']')[0];
                var at = atEl ? resolveValue(atEl, $(atEl).children('block:first').attr('type')) : '1';
                if (where == 'FIRST') return '(function(t) if #t==0 then return nil end return t[1] end)' + '(' + list + ')'
                if (where == 'LAST')  return '(function(t) if #t==0 then return nil end return t[#t] end)' + '(' + list + ')'
                if (where == 'RANDOM')return '(function(t) if #t==0 then return nil end return t[math.random(1,#t)] end)' + '(' + list + ')'
                // FROM_START / FROM_END with AT number (1-based in Blockly)
                var idx = where == 'FROM_END' ? '(#' + list + ' - ' + at + ' + 1)' : at;
                return '(function(t,i) if i<1 or i>#t then return nil end return t[i] end)' + '(' + list + ',' + idx + ')'
            }
            else if (variableType == 'lists_setIndex') {
                var list = _val('LIST'), mode = _fld('MODE'), where = _fld('WHERE');
                var atEl = $(_b).children('value[name=\'AT\']')[0];
                var at = atEl ? resolveValue(atEl, $(atEl).children('block:first').attr('type')) : '1';
                var toEl = $(_b).children('value[name=\'TO\']')[0];
                var to = toEl ? resolveValue(toEl, $(toEl).children('block:first').attr('type')) : 'nil';
                if (where == 'FIRST') {
                    if (mode == 'INSERT') return '(function(t,x) local r={x} for i=1,#t do r[#r+1]=t[i] end return r end)' + '(' + list + ',' + to + ')'
                    return '(function(t,x) local r={} for i=1,#t do r[i]=t[i] end r[1]=x return r end)' + '(' + list + ',' + to + ')' // SET
                }
                if (where == 'LAST') {
                    if (mode == 'INSERT') return '(function(t,x) local r={} for i=1,#t do r[i]=t[i] end r[#r+1]=x return r end)' + '(' + list + ',' + to + ')'
                    return '(function(t,x) if #t==0 then return {x} end local r={} for i=1,#t do r[i]=t[i] end r[#r]=x return r end)' + '(' + list + ',' + to + ')' // SET
                }
                var idx = where == 'FROM_END' ? '(#' + list + ' - ' + at + ' + 1)' : at;
                if (mode == 'INSERT') return '(function(t,i,x) local r={} for j=1,#t do if j==i then r[#r+1]=x end r[#r+1]=t[j] end return r end)' + '(' + list + ',' + idx + ',' + to + ')'
                return '(function(t,i,x) local r={} for j=1,#t do r[j]=(j==i and x or t[j]) end return r end)' + '(' + list + ',' + idx + ',' + to + ')' // SET
            }
            else if (variableType == 'lists_sort') {
                var list = _val('LIST'), type = _fld('TYPE'), dir = _fld('DIRECTION');
                // table.sort needs a strict "a comes before b" comparator; normalize both operands per TYPE
                var sym = dir == '-1' ? '>' : '<';
                var less;
                if (type == 'NUMERIC') less = '(tonumber(a) or 0)' + sym + '(tonumber(b) or 0)';
                else if (type == 'IGNORE_CASE') less = 'tostring(a):lower()' + sym + 'tostring(b):lower()';
                else less = 'tostring(a)' + sym + 'tostring(b)'; // TEXT
                var order = 'function(a,b) return ' + less + ' end';
                return '(function(t) local s={} for i=1,#t do s[i]=t[i] end table.sort(s,' + order + ') return s end)' + '(' + list + ')'
            }
            else if (variableType == 'lists_getSublist') {
                var list = _val('LIST');
                var ws = _fldOrMut('WHERE1', 'where1', 'FIRST'), we = _fldOrMut('WHERE2', 'where2', 'LAST');
                // Blockly mutates the AT1/AT2 dummy inputs into value inputs when FROM_START/FROM_END is selected.
                var from = _val('AT1'); if (from === '') from = '1';
                var to = _val('AT2'); if (to === '') to = '#' + list;
                // Compute 1-based start/end indices (Blockly is 1-based).
                var startIdx, endIdx;
                if (ws == 'FIRST') startIdx = '1';
                else if (ws == 'FROM_END') startIdx = '(#' + list + ' - ' + from + ' + 1)';
                else startIdx = from; // FROM_START
                if (we == 'LAST') endIdx = '#' + list;
                else if (we == 'FROM_END') endIdx = '(#' + list + ' - ' + to + ' + 1)';
                else endIdx = to; // FROM_START
                return '(function(t,a,b) local r={} for i=a,b do if t[i]~=nil then r[#r+1]=t[i] end end return r end)' + '(' + list + ',' + startIdx + ',' + endIdx + ')'
            }
            else if (variableType == 'lists_reverse') {
                return _rev(_val('LIST'));
            }
            else if (variableType == 'lists_split') {
                var mode = _fldOrMut('MODE', 'mode', 'SPLIT');
                if (mode == 'JOIN') {
                    var list = _val('INPUT'), delim = _val('DELIM');
                    return '(function(t,d) local r="" for i=1,#t do r=r..(i>1 and d or "")..tostring(t[i]) end return r end)' + '(' + list + ',' + delim + ')'
                }
                // SPLIT: split string by delimiter -> array (Lua 5.4 has no table.split; emulate)
                var str = _val('INPUT'), delim = _val('DELIM');
                return '(function(s,d) if d=="" then local r={} for i=1,#s do r[i]=s:sub(i,i) end r[#r+1]="" return r end local r={} local i=1 while true do local p=s:find(d,i,true) if not p then r[#r+1]=s:sub(i) break end r[#r+1]=s:sub(i,p-1) i=p+#d end return r end)' + '(' + str + ',' + delim + ')'
            }
            else if (variableType == 'text_join') {
                // Saved XML carries the joined operands as ADD0/ADD1/... value inputs on the block itself;
                // text_create_join_item blocks exist only inside the mutator sub-workspace.
                var parts = [];
                $(_b).children('value').each(function () {
                    var bt = $(this).children('block:first').attr('type');
                    if (bt) parts.push(resolveValue(this, bt));
                });
                if (!parts.length) return '""';
                return '(' + parts.join('..') + ')';
            }
            else if (variableType == 'text_append') {
                // v13: field VAR (a variable name) + value input TEXT.
                var txt = _val('TEXT');
                var vn = _fld('VAR');
                return '(' + (vn ? ('variable[\'' + vn + '\']') : '""') + '..' + txt + ')';
            }
            else if (variableType == 'text_length') {
                return '#' + _val('VALUE');
            }
            else if (variableType == 'text_isEmpty') {
                return '(' + _val('VALUE') + ')==""';
            }
            else if (variableType == 'text_charAt') {
                var val = _val('VALUE'), where = _fld('WHERE');
                if (where == 'FIRST')  return '(function(s) return s:sub(1,1) end)' + '(' + val + ')'
                if (where == 'LAST')   return '(function(s) return s:sub(-1,-1) end)' + '(' + val + ')'
                if (where == 'FROM_START') return '(function(s,i) if i<1 or i>#s then return "" end return s:sub(i,i) end)' + '(' + val + ',' + _val('AT') + ')'
                if (where == 'FROM_END')   return '(function(s,i) local j=#s-i+1 if j<1 or j>#s then return "" end return s:sub(j,j) end)' + '(' + val + ',' + _val('AT') + ')'
                return '(function(s) local i=math.random(1,#s) return s:sub(i,i) end)' + '(' + val + ')' // RANDOM
            }
            else if (variableType == 'text_count') {
                var sub = _val('SUB'), txt = _val('TEXT');
                return '(function(s,x) if x=="" then return 0 end local c=0 local i=1 while true do local p=s:find(x,i,true) if not p then break end c=c+1 i=p+#x end return c end)' + '(' + txt + ',' + sub + ')'
            }
            else if (variableType == 'text_indexOf') {
                var val = _val('VALUE'), find = _val('FIND'), end_ = _fld('END');
                if (end_ == 'LAST') return '(function(s,x) local r=0 for i=#s-#x+1,1,-1 do if s:sub(i,i+#x-1)==x then r=i break end end return r end)' + '(' + val + ',' + find + ')'
                return '(function(s,x) local p=s:find(x,1,true) return p or 0 end)' + '(' + val + ',' + find + ')' // FIRST
            }
            else if (variableType == 'text_getSubstring') {
                var val = _val('STRING');
                var ws = _fldOrMut('WHERE1', 'where1', 'FIRST'), we = _fldOrMut('WHERE2', 'where2', 'LAST');
                // Blockly mutates the AT1/AT2 dummy inputs into value inputs when FROM_START/FROM_END is selected.
                var from = _val('AT1'); if (from === '') from = '1';
                var to = _val('AT2'); if (to === '') to = '#' + val;
                var startIdx, endIdx;
                if (ws == 'FIRST') startIdx = '1';
                else if (ws == 'FROM_END') startIdx = '(#' + val + ' - ' + from + ' + 1)';
                else startIdx = from; // FROM_START
                if (we == 'LAST') endIdx = '#' + val;
                else if (we == 'FROM_END') endIdx = '(#' + val + ' - ' + to + ' + 1)';
                else endIdx = to; // FROM_START
                return '(function(s,a,b) if a<1 then a=1 end if b>#s then b=#s end if a>b then return "" end return s:sub(a,b) end)' + '(' + val + ',' + startIdx + ',' + endIdx + ')'
            }
            else if (variableType == 'text_changeCase') {
                var op = _fld('CASE'), txt = _val('TEXT');
                if (op == 'UPPERCASE') return 'string.upper(' + txt + ')';
                if (op == 'LOWERCASE') return 'string.lower(' + txt + ')';
                // TITLECASE: capitalize first letter of each word
                return '(function(s) local r="" for i=1,#s do local c=s:sub(i,i) if i==1 or s:sub(i-1,i-1)==" " then r=r..string.upper(c) else r=r..c end end return r end)' + '(' + txt + ')'
            }
            else if (variableType == 'text_replace') {
                var from = _val('FROM'), to = _val('TO'), txt = _val('TEXT');
                return '(function(s,a,b) if a=="" then return s end local r="" local i=1 while true do local p=s:find(a,i,true) if not p then r=r..s:sub(i) break end r=r..s:sub(i,p-1)..b i=p+#a end return r end)' + '(' + txt + ',' + from + ',' + to + ')'
            }
            else if (variableType == 'text_reverse') {
                var txt = _val('TEXT');
                return '(function(s) local r="" for i=#s,1,-1 do r=r..s:sub(i,i) end return r end)' + '(' + txt + ')'
            }
            else if (variableType == 'text_trim') {
                var op = _fld('MODE'), txt = _val('TEXT');
                if (op == 'LEFT')  return '(function(s) return s:match("^%s*(.-)$") end)' + '(' + txt + ')'
                if (op == 'RIGHT') return '(function(s) return s:match("^(.-)%s*$") end)' + '(' + txt + ')'
                return '(function(s) return s:match("^%s*(.-)%s*$") end)' + '(' + txt + ')' // BOTH
            }
            else if (variableType == 'url_text') {
                var f = $(_b).children('field[name=\'TEXT\']')[0];
                if (f) return '"http://' + $(f).text() + '"';
            }
            return GetValueText(value, variableType);
        }

        function parseLogicCompare(thisBlock) {
            var locOperand = opSymbol($($(thisBlock).children('field:first')).text());
            var valueA = $(thisBlock).children('value[name=\'A\']')[0];
            var variableTypeA = $(valueA).children('block:first').attr('type');
            var valueB = $(thisBlock).children('value[name=\'B\']')[0];
            var variableTypeB = $(valueB).children('block:first').attr('type');
            var varTextA = resolveValue(valueA, variableTypeA);
            var varTextB = resolveValue(valueB, variableTypeB);

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
                var totalminutes = 9999;
                var res = $(valueA).text().split(":");
                if (res.length == 2) {
					var hours = parseInt(res[0]);
					var minutes = parseInt(res[1]);
					totalminutes = (hours * 60) + minutes;
                }
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
            else {
                // Plain numbers / math blocks (v13 standard blocks) as minutes since midnight
                var timeText = resolveValue(valueTime, timeBlock.attr('type'));
                if (timeText) {
                    compareString = 'timeofday ' + locOperand + ' ' + timeText;
                }
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
            else if (firstBlock.attr('type') == 'math_arithmetic') {
                var conditionstring = resolveValue(thisBlock, firstBlock.attr('type'));
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
                    else if (blockB.attr('type') == 'math_arithmetic') {
                        // Actions are parsed as command strings by EventSystem.cpp, not evaluated as Lua:
                        // a raw expression would be sent to the device literally. Reject it clearly.
                        throw new Error('Arithmetic results cannot be used as a device action; use a User variable instead');
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
                var logType = blockInfo.attr('type');
                if (!logType) {
                    throw new Error('Write to log block has no text attached');
                }
                var setString = '';
                if (logType == 'text') {
                    var logText = $(blockInfo).find('field[name=\'TEXT\']')[0];
                    setString = 'commandArray["WriteToLogText"]="' + $(logText).text() + '"';
                    setArray.push(setString);
                }
                else if (logType.indexOf('uservariables') >= 0) {
                    var userVar = $(blockInfo).find('field[name=\'Variable\']')[0];
                    setString = 'commandArray["WriteToLogUserVariable"]="' + $(userVar).text() + '"';
                    setArray.push(setString);
                }
                else if (logType.indexOf('switchvariables') >= 0) {
                    var switchVar = $(blockInfo).find('field')[0];
                    setString = 'commandArray["WriteToLogSwitch"]="' + $(switchVar).text() + '"';
                    setArray.push(setString);
                }
                else if (logType.indexOf('variables') >= 0) {
                    var deviceVar = $(blockInfo).find('field')[0];
                    setString = 'commandArray["WriteToLogDeviceVariable"]="' + $(deviceVar).text() + '"';
                    setArray.push(setString);
                }
                else {
                    throw new Error('Write to log only supports text or a device/switch/user variable');
                }
            }
            else if ($(this).attr('type') == 'text_print') {
                // v13 standard "print" block -> log message (server: commandArray["WriteToLogText"])
                var textValue = $(this).find('value[name=\'TEXT\']')[0];
                var textBlockInfo = $(textValue).children('block:first');
                var printType = textBlockInfo.attr('type');
                if (!printType) {
                    throw new Error('Print block has no text attached');
                }
                if (printType == 'text') {
                    var printText = $(textBlockInfo).find('field[name=\'TEXT\']')[0];
                    setArray.push('commandArray["WriteToLogText"]="' + $(printText).text() + '"');
                }
                else if (printType.indexOf('uservariables') >= 0) {
                    var printVar = $(textBlockInfo).find('field[name=\'Variable\']')[0];
                    setArray.push('commandArray["WriteToLogUserVariable"]="' + $(printVar).text() + '"');
                }
                else if (printType.indexOf('switchvariables') >= 0) {
                    var printSwitch = $(textBlockInfo).find('field')[0];
                    setArray.push('commandArray["WriteToLogSwitch"]="' + $(printSwitch).text() + '"');
                }
                else if (printType.indexOf('variables') >= 0) {
                    var printDevice = $(textBlockInfo).find('field')[0];
                    setArray.push('commandArray["WriteToLogDeviceVariable"]="' + $(printDevice).text() + '"');
                }
                else {
                    throw new Error('Print only supports text or a device/switch/user variable');
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
