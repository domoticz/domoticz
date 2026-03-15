#!/bin/bash
#
# Integration tests for plugin Settings lifecycle
# Tests orphan cleanup, default addition, and value preservation across restarts.
# Requires: domoticz binary built at project root, sqlite3, curl, python3
#
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
DOMOTICZ="$PROJECT_DIR/domoticz"
DB="/tmp/domoticz-lifecycle-test.db"
LOG="/tmp/domoticz-lifecycle-test.log"
PORT=8090
PLUGIN_DIR="$PROJECT_DIR/plugins/TestExtendedSettings"
PLUGIN_FILE="$PLUGIN_DIR/plugin.py"
PLUGIN_BACKUP="$PLUGIN_FILE.bak"
BASE="http://localhost:$PORT"
PASS=0
FAIL=0
DOMOTICZ_PID=""

cleanup() {
	if [ -n "$DOMOTICZ_PID" ] && kill -0 "$DOMOTICZ_PID" 2>/dev/null; then
		kill "$DOMOTICZ_PID" 2>/dev/null
		wait "$DOMOTICZ_PID" 2>/dev/null || true
	fi
	rm -f "$DB" "$LOG" "$DB-shm" "$DB-wal"
	if [ -f "$PLUGIN_BACKUP" ]; then
		cp "$PLUGIN_BACKUP" "$PLUGIN_FILE"
		rm -f "$PLUGIN_BACKUP"
	fi
}

trap cleanup EXIT

start_domoticz() {
	if [ -n "$DOMOTICZ_PID" ] && kill -0 "$DOMOTICZ_PID" 2>/dev/null; then
		kill "$DOMOTICZ_PID" 2>/dev/null
		wait "$DOMOTICZ_PID" 2>/dev/null || true
	fi
	"$DOMOTICZ" -sslwww 0 -wwwroot www -dbase "$DB" -www "$PORT" >> "$LOG" 2>&1 &
	DOMOTICZ_PID=$!
	# Poll for startup with timeout
	local retries=0
	while [ $retries -lt 20 ]; do
		if curl -s "$BASE/json.htm?type=command&param=getversion" > /dev/null 2>&1; then
			# Wait for plugin system to finish starting hardware
			sleep 2
			return 0
		fi
		sleep 0.5
		retries=$((retries+1))
	done
	echo "FATAL: domoticz failed to start within 10 seconds"
	cat "$LOG"
	exit 1
}

setup_trusted() {
	sqlite3 "$DB" "INSERT OR REPLACE INTO Preferences (Key, nValue, sValue) VALUES ('WebLocalNetworks', 0, '127.0.0.;::1');"
}

check() {
	local desc="$1"
	local result="$2"
	echo -n "$desc: "
	if echo "$result" | grep -q "^PASS"; then
		echo "PASS"
		PASS=$((PASS+1))
	else
		echo "FAIL — $result"
		FAIL=$((FAIL+1))
	fi
}

echo "=== Plugin Settings Lifecycle Integration Tests ==="
echo ""

# Save original plugin
cp "$PLUGIN_FILE" "$PLUGIN_BACKUP"

# --- Test 1: Fresh install persists defaults to DB ---
echo "--- Test 1: Fresh install persists defaults to DB ---"
rm -f "$DB"
start_domoticz
setup_trusted
start_domoticz

# Add hardware with empty settings
curl -s "$BASE/json.htm?type=command&param=addhardware&htype=94&name=LifecycleTest&enabled=true&datatimeout=0&loglevel=0&address=&port=0&serialport=&username=&password=&extra=TestExtSettings&Mode1=&Mode2=&Mode3=&Mode4=&Mode5=&Mode6=&settings=" > /dev/null

# The plugin start should persist defaults. Need to restart for plugin to load.
start_domoticz

# Check DB has defaults
RESULT=$(python3 -c "
import json, sqlite3
conn = sqlite3.connect('$DB')
row = conn.execute(\"SELECT Settings FROM Hardware WHERE Extra='TestExtSettings'\").fetchone()
conn.close()
if not row or not row[0]:
    print('FAIL: Settings empty')
else:
    s = json.loads(row[0])
    if s.get('Interval') == '30' and s.get('EnableDebug') == 'false' and s.get('Brightness') == '50':
        print('PASS')
    else:
        print('FAIL: ' + json.dumps(s))
")
check "Fresh defaults persisted to DB" "$RESULT"

# Also verify via API
RESULT=$(curl -s "$BASE/json.htm?type=command&param=gethardware" | python3 -c "
import sys,json
d=json.load(sys.stdin)
for h in d.get('result',[]):
    if h.get('Extra')=='TestExtSettings':
        s=h.get('Settings',{})
        if s.get('Interval')=='30' and s.get('RetryCount')=='3':
            print('PASS')
        else:
            print('FAIL: ' + json.dumps(s))
        break
else:
    print('FAIL: hardware not found')
")
check "Defaults visible in API response" "$RESULT"

# --- Test 2: Stored values not overwritten by changed defaults ---
echo ""
echo "--- Test 2: Stored values preserved when default changes ---"

# Update Interval to 120
IDX=$(curl -s "$BASE/json.htm?type=command&param=gethardware" | python3 -c "
import sys,json
d=json.load(sys.stdin)
for h in d.get('result',[]):
    if h.get('Extra')=='TestExtSettings': print(h['idx']); break
")
SETTINGS=$(python3 -c "import urllib.parse,json; print(urllib.parse.quote(json.dumps({'Interval':'120','EnableDebug':'true','Brightness':'75','Protocol':'http','Certificate':'','ApiKey':'','RetryCount':'3','Timeout':'10','EnableNotifications':'true'})))")
curl -s "$BASE/json.htm?type=command&param=updatehardware&htype=94&idx=$IDX&name=LifecycleTest&enabled=true&datatimeout=0&loglevel=0&address=&port=0&serialport=&username=&password=&extra=TestExtSettings&Mode1=&Mode2=&Mode3=&Mode4=&Mode5=&Mode6=&settings=$SETTINGS" > /dev/null

# Modify plugin XML: change Interval default from 30 to 90
sed -i 's/default="30"/default="90"/' "$PLUGIN_FILE"

# Restart
start_domoticz

# Stored value (120) should be preserved, not overwritten by new default (90)
RESULT=$(curl -s "$BASE/json.htm?type=command&param=gethardware" | python3 -c "
import sys,json
d=json.load(sys.stdin)
for h in d.get('result',[]):
    if h.get('Extra')=='TestExtSettings':
        s=h.get('Settings',{})
        if s.get('Interval')=='120':
            print('PASS')
        else:
            print('FAIL: Interval=' + repr(s.get('Interval')) + ' (expected 120)')
        break
")
check "Stored value preserved after default change" "$RESULT"

# --- Test 3: New field gets default on restart ---
echo ""
echo "--- Test 3: New field gets default on restart ---"

# Add a new field to plugin XML
sed -i '/<param field="Interval"/a\        <param field="NewParam" type="text" label="New Parameter" default="hello_world" width="200px"/>' "$PLUGIN_FILE"

# Restart
start_domoticz

# NewParam should appear with default value, existing values preserved
RESULT=$(curl -s "$BASE/json.htm?type=command&param=gethardware" | python3 -c "
import sys,json
d=json.load(sys.stdin)
for h in d.get('result',[]):
    if h.get('Extra')=='TestExtSettings':
        s=h.get('Settings',{})
        if s.get('NewParam')=='hello_world' and s.get('Interval')=='120':
            print('PASS')
        else:
            print('FAIL: NewParam=' + repr(s.get('NewParam')) + ' Interval=' + repr(s.get('Interval')))
        break
")
check "New field gets default, existing values preserved" "$RESULT"

# --- Test 4: Removed field cleaned from Settings ---
echo ""
echo "--- Test 4: Removed field cleaned from Settings ---"

# Remove NewParam from plugin XML
sed -i '/NewParam/d' "$PLUGIN_FILE"

# Restart
start_domoticz

# NewParam should be gone, other fields preserved
RESULT=$(curl -s "$BASE/json.htm?type=command&param=gethardware" | python3 -c "
import sys,json
d=json.load(sys.stdin)
for h in d.get('result',[]):
    if h.get('Extra')=='TestExtSettings':
        s=h.get('Settings',{})
        if 'NewParam' not in s and s.get('Interval')=='120':
            print('PASS')
        else:
            print('FAIL: NewParam still present or Interval wrong: ' + json.dumps(s))
        break
")
check "Removed field cleaned from Settings" "$RESULT"

# --- Test 5: Verify DB is clean after orphan removal ---
echo ""
echo "--- Test 5: DB Settings clean after orphan removal ---"
RESULT=$(python3 -c "
import json, sqlite3
conn = sqlite3.connect('$DB')
row = conn.execute(\"SELECT Settings FROM Hardware WHERE Extra='TestExtSettings'\").fetchone()
conn.close()
if not row or not row[0]:
    print('FAIL: Settings empty')
else:
    s = json.loads(row[0])
    if 'NewParam' not in s and s.get('Interval')=='120':
        print('PASS')
    else:
        print('FAIL: ' + json.dumps(s))
")
check "DB Settings clean after orphan removal" "$RESULT"

echo ""
echo "=============================="
echo "Lifecycle tests: $PASS passed, $FAIL failed"

# Cleanup handled by trap
exit $FAIL
