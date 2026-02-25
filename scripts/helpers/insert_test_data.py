"""
insert_test_data.py - Insert test sensor data into domoticz.db for validating
compare chart fixes (issue #6591).

Inserts 24 months of calendar + short-log data for multiple sensor types.
Domoticz must be STOPPED before running this script.

Usage:
    python scripts/helpers/insert_test_data.py
"""

import sqlite3
import os
import sys
from datetime import date, datetime, timedelta
from dateutil.relativedelta import relativedelta

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Path to the database (adjust if needed)
DB_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "domoticz.db")

# Hardware ID for test devices
HARDWARE_ID = 28

# Enable/disable which sensor types to insert (set to False to skip)
INSERT_USAGE       = True
INSERT_WEIGHT      = True
INSERT_RAIN        = True
INSERT_WIND        = True
INSERT_VOLTAGE     = True
INSERT_WATER       = True
INSERT_KWH         = True
INSERT_P1_POWER    = True
INSERT_P1_GAS      = True
INSERT_CUSTOM      = True
INSERT_TEMPERATURE = True

# ---------------------------------------------------------------------------
# Type/SubType constants (from hardwaretypes.h / RFXtrx.h)
# ---------------------------------------------------------------------------

pTypeUsage       = 0xF8
sTypeElectric    = 0x01

pTypeWEIGHT      = 0x5D
sTypeWEIGHT1     = 0x01

pTypeRAIN        = 0x55
sTypeRAIN1       = 0x01

pTypeWIND        = 0x56
sTypeWIND1       = 0x01

pTypeGeneral     = 0xF3
sTypeVoltage     = 0x08
sTypeKwh         = 0x1D
sTypeCustom      = 0x1F

pTypeP1Power     = 0xFA
sTypeP1Power     = 0x01

pTypeP1Gas       = 0xFB
sTypeP1Gas       = 0x02

pTypeRFXMeter    = 0x71
sTypeRFXMeterCount = 0x00

pTypeTEMP        = 0x50
sTypeTEMP1       = 0x01

# ---------------------------------------------------------------------------
# Device definitions
# ---------------------------------------------------------------------------

DEVICES = []

if INSERT_USAGE:
    DEVICES.append({
        "name":        "Test Usage Sensor",
        "device_id":   "00000001",
        "unit":        1,
        "type":        pTypeUsage,
        "subtype":     sTypeElectric,
        "switch_type": 0,
        "s_value":     "300",
        "options":     None,
        "calendar":    "MultiMeter_Calendar",
        "shortlog":    "Meter",
    })

if INSERT_WEIGHT:
    DEVICES.append({
        "name":        "Test Weight Sensor",
        "device_id":   "00000002",
        "unit":        1,
        "type":        pTypeWEIGHT,
        "subtype":     sTypeWEIGHT1,
        "switch_type": 0,
        "s_value":     "5.4",
        "options":     None,
        "calendar":    "MultiMeter_Calendar",
        "shortlog":    "Meter",
    })

if INSERT_RAIN:
    DEVICES.append({
        "name":        "Test Rain Sensor",
        "device_id":   "00000003",
        "unit":        1,
        "type":        pTypeRAIN,
        "subtype":     sTypeRAIN1,
        "switch_type": 0,
        "s_value":     "0;0",
        "options":     None,
        "calendar":    "Rain_Calendar",
        "shortlog":    "Rain",
    })

if INSERT_WIND:
    DEVICES.append({
        "name":        "Test Wind Sensor",
        "device_id":   "00000004",
        "unit":        1,
        "type":        pTypeWIND,
        "subtype":     sTypeWIND1,
        "switch_type": 0,
        "s_value":     "0;N;0;0;0;0",
        "options":     None,
        "calendar":    "Wind_Calendar",
        "shortlog":    "Wind",
    })

if INSERT_VOLTAGE:
    DEVICES.append({
        "name":        "Test Voltage Sensor",
        "device_id":   "00000005",
        "unit":        1,
        "type":        pTypeGeneral,
        "subtype":     sTypeVoltage,
        "switch_type": 0,
        "s_value":     "230.0",
        "options":     None,
        "calendar":    "MultiMeter_Calendar",
        "shortlog":    "Meter",
    })

if INSERT_WATER:
    DEVICES.append({
        "name":        "Test Water Sensor",
        "device_id":   "00000006",
        "unit":        1,
        "type":        pTypeRFXMeter,
        "subtype":     sTypeRFXMeterCount,
        "switch_type": 2,  # MTYPE_WATER
        "s_value":     "12345",
        "options":     None,
        "calendar":    "Meter_Calendar",
        "shortlog":    "Meter",
    })

if INSERT_KWH:
    DEVICES.append({
        "name":        "Test kWh Sensor",
        "device_id":   "00000007",
        "unit":        1,
        "type":        pTypeGeneral,
        "subtype":     sTypeKwh,
        "switch_type": 0,
        "s_value":     "150;12345",
        "options":     None,
        "calendar":    "Meter_Calendar",
        "shortlog":    "Meter",
    })

if INSERT_P1_POWER:
    DEVICES.append({
        "name":        "Test P1 Power Sensor",
        "device_id":   "00000008",
        "unit":        1,
        "type":        pTypeP1Power,
        "subtype":     sTypeP1Power,
        "switch_type": 0,
        "s_value":     "100;200;150;250;500;600",
        "options":     None,
        "calendar":    "MultiMeter_Calendar",
        "shortlog":    "MultiMeter",
    })

if INSERT_P1_GAS:
    DEVICES.append({
        "name":        "Test P1 Gas Sensor",
        "device_id":   "00000009",
        "unit":        1,
        "type":        pTypeP1Gas,
        "subtype":     sTypeP1Gas,
        "switch_type": 0,
        "s_value":     "54321",
        "options":     None,
        "calendar":    "Meter_Calendar",
        "shortlog":    "Meter",
    })

if INSERT_CUSTOM:
    DEVICES.append({
        "name":        "Test Custom Sensor",
        "device_id":   "0000000A",
        "unit":        1,
        "type":        pTypeGeneral,
        "subtype":     sTypeCustom,
        "switch_type": 0,
        "s_value":     "42.5",
        "options":     "1;ppm",
        "calendar":    "Percentage_Calendar",
        "shortlog":    "Percentage",
    })

if INSERT_TEMPERATURE:
    DEVICES.append({
        "name":        "Test Temperature Sensor",
        "device_id":   "0000000B",
        "unit":        1,
        "type":        pTypeTEMP,
        "subtype":     sTypeTEMP1,
        "switch_type": 0,
        "s_value":     "18.5",
        "options":     None,
        "calendar":    "Temperature_Calendar",
        "shortlog":    "Temperature",
    })

# ---------------------------------------------------------------------------
# Date range: 24 months ending at the current month (one per month on the 1st)
# Plus daily entries for the last 60 days for the month/week charts.
# ---------------------------------------------------------------------------

TODAY = date.today()
MONTH_END = date(TODAY.year, TODAY.month, 1)  # first of current month
MONTH_START = MONTH_END - relativedelta(months=23)

MONTHS = []
d = MONTH_START
while d <= MONTH_END:
    MONTHS.append(d)
    d += relativedelta(months=1)

# Daily entries for the last 60 days (excluding dates already in MONTHS)
DAILY_DAYS = 60
DAILY_DATES = []
for i in range(DAILY_DAYS, 0, -1):
    day = TODAY - timedelta(days=i)
    if day not in MONTHS:
        DAILY_DATES.append(day)

# Combined calendar dates = monthly + daily (sorted, deduplicated)
ALL_CALENDAR_DATES = sorted(set(MONTHS + DAILY_DATES))

# Short-log: generate 5 entries per day for the last 7 days
SHORTLOG_DAYS = 7
SHORTLOG_PER_DAY = 5

def shortlog_timestamps():
    """Generate short-log datetime strings for the last SHORTLOG_DAYS days."""
    timestamps = []
    now = datetime.now()
    for day_offset in range(SHORTLOG_DAYS, 0, -1):
        day = now - timedelta(days=day_offset)
        for i in range(SHORTLOG_PER_DAY):
            hour = 6 + i * 3  # 06:00, 09:00, 12:00, 15:00, 18:00
            ts = day.replace(hour=hour, minute=0, second=0, microsecond=0)
            timestamps.append(ts.strftime("%Y-%m-%d %H:%M:%S"))
    return timestamps

SHORTLOG_TIMES = shortlog_timestamps()

# ---------------------------------------------------------------------------
# Per-month value generators (calendar data)
# ---------------------------------------------------------------------------

def seasonal(month, winter_val, summer_val, day=1):
    """Return a value that varies seasonally (higher in winter, lower in summer)
    with slight daily variation."""
    # month 1=Jan(winter peak), 7=Jul(summer trough)
    factor = abs(month - 7) / 6.0  # 1.0 in Jan, 0.0 in Jul
    base = winter_val * factor + summer_val * (1 - factor)
    # Add small daily jitter (~5% range based on day-of-month)
    jitter = ((day % 7) - 3) / 60.0  # -0.05 to +0.05
    return base * (1 + jitter)


def gen_calendar_usage(month_date):
    """MultiMeter_Calendar row for Usage sensor. Value2 = watts * 10."""
    v2 = int(seasonal(month_date.month, 3200, 2800, month_date.day))
    return {"Value1": 0, "Value2": v2, "Value3": 0, "Value4": 0, "Value5": 0, "Value6": 0}


def gen_calendar_weight(month_date):
    """MultiMeter_Calendar row for Weight sensor. Values = kg * 10."""
    avg = int(seasonal(month_date.month, 58, 50, month_date.day))
    return {"Value1": avg - 4, "Value2": avg + 4, "Value3": avg, "Value4": 0, "Value5": 0, "Value6": 0}


def gen_calendar_rain(month_date):
    """Rain_Calendar row. Total in mm."""
    total = round(seasonal(month_date.month, 70.0, 30.0, month_date.day) / 30.0, 1)  # daily portion
    return {"Total": total, "Rate": 0}


def gen_calendar_wind(month_date):
    """Wind_Calendar row. Speed/Gust stored as m/s * 10."""
    speed_max = int(seasonal(month_date.month, 62, 28, month_date.day))
    gust_max = int(speed_max * 1.5)
    speed_min = max(0, speed_max - 20)
    gust_min = max(0, gust_max - 25)
    return {"Direction": (month_date.day * 15) % 360, "Speed_Min": speed_min,
            "Speed_Max": speed_max, "Gust_Min": gust_min, "Gust_Max": gust_max}


def gen_calendar_voltage(month_date):
    """MultiMeter_Calendar row for Voltage. Stored as V * 1000."""
    avg = int(seasonal(month_date.month, 232000, 228000, month_date.day))
    return {"Value1": avg - 2000, "Value2": avg + 2000, "Value3": avg, "Value4": 0, "Value5": 0, "Value6": 0}


def gen_calendar_water(month_date):
    """Meter_Calendar row for Water. Value = litres."""
    val = int(seasonal(month_date.month, 8000, 12000, month_date.day) / 30.0)  # daily portion
    return {"Value": val, "Counter": 0}


def gen_calendar_kwh(month_date):
    """Meter_Calendar row for kWh. Value=usage Wh per day."""
    usage = int(seasonal(month_date.month, 12000, 6000, month_date.day))
    return {"Value": usage, "Counter": 0}


def gen_calendar_p1power(month_date):
    """MultiMeter_Calendar row for P1 Power.
    Value1=usage1(low), Value2=usage2(high), Value5=return1, Value6=return2."""
    base = int(seasonal(month_date.month, 400000, 200000, month_date.day) / 30.0)  # daily portion
    return {"Value1": base, "Value2": int(base * 0.6), "Value3": 0, "Value4": 0,
            "Value5": int(base * 0.1), "Value6": int(base * 0.05)}


def gen_calendar_p1gas(month_date):
    """Meter_Calendar row for P1 Gas. Value in dm3 (0.001 m3)."""
    val = int(seasonal(month_date.month, 200000, 20000, month_date.day) / 30.0)  # daily portion
    return {"Value": val, "Counter": 0}


def gen_calendar_custom(month_date):
    """Percentage_Calendar row for Custom sensor."""
    avg = seasonal(month_date.month, 55.0, 35.0, month_date.day)
    return {"Percentage_Min": avg - 5, "Percentage_Max": avg + 5, "Percentage_Avg": avg}


def gen_calendar_temperature(month_date):
    """Temperature_Calendar row."""
    avg = seasonal(month_date.month, 2.0, 22.0, month_date.day)
    return {"Temp_Min": avg - 3, "Temp_Max": avg + 3, "Temp_Avg": avg,
            "Chill_Min": avg - 5, "Chill_Max": avg - 2,
            "Humidity": int(seasonal(month_date.month, 85, 55, month_date.day)),
            "Barometer": 1013, "DewPoint": avg - 4,
            "SetPoint_Min": 0, "SetPoint_Max": 0, "SetPoint_Avg": 0}


# Map device name -> calendar generator
CALENDAR_GENERATORS = {
    "Test Usage Sensor":       gen_calendar_usage,
    "Test Weight Sensor":      gen_calendar_weight,
    "Test Rain Sensor":        gen_calendar_rain,
    "Test Wind Sensor":        gen_calendar_wind,
    "Test Voltage Sensor":     gen_calendar_voltage,
    "Test Water Sensor":       gen_calendar_water,
    "Test kWh Sensor":         gen_calendar_kwh,
    "Test P1 Power Sensor":    gen_calendar_p1power,
    "Test P1 Gas Sensor":      gen_calendar_p1gas,
    "Test Custom Sensor":      gen_calendar_custom,
    "Test Temperature Sensor": gen_calendar_temperature,
}

# ---------------------------------------------------------------------------
# Short-log value generators
# ---------------------------------------------------------------------------

def _shortlog_index(ts_str):
    """Return a 0-based index from the timestamp string for generating variation."""
    # Parse hour and day from 'YYYY-MM-DD HH:MM:SS'
    parts = ts_str.split()
    day = int(parts[0].split('-')[2])
    hour = int(parts[1].split(':')[0])
    return (day * 24 + hour)


def gen_shortlog_usage(ts_str):
    """Meter short-log row for Usage. sValue*10 stored in Value."""
    i = _shortlog_index(ts_str)
    value = 2500 + (i % 10) * 100  # 250-350W range (*10)
    return {"Value": value, "Usage": 0}


def gen_shortlog_weight(ts_str):
    """Meter short-log row for Weight. sValue*10 stored in Value."""
    i = _shortlog_index(ts_str)
    value = 50 + (i % 9)  # 5.0-5.8 kg range (*10)
    return {"Value": value, "Usage": 0}


# Rain Total must be cumulative (day chart computes MAX-MIN)
_rain_base_total = 1000.0  # starting cumulative total in mm

def gen_shortlog_rain(ts_str):
    """Rain short-log: Total must be cumulative, Rate is instantaneous."""
    i = _shortlog_index(ts_str)
    total = _rain_base_total + i * 0.3  # ~0.3mm per entry
    rate = 1 + (i % 5)  # varying rate
    return {"Total": round(total, 1), "Rate": rate}


def gen_shortlog_wind(ts_str):
    """Wind short-log: Direction, Speed (*10 m/s), Gust (*10 m/s)."""
    i = _shortlog_index(ts_str)
    direction = (i * 30) % 360  # rotating direction
    speed = 20 + (i % 15) * 3  # 2.0-6.2 m/s range (*10)
    gust = speed + 10 + (i % 8) * 2  # gust > speed
    return {"Direction": float(direction), "Speed": speed, "Gust": gust}


def gen_shortlog_voltage(ts_str):
    """Meter short-log for Voltage. Stored as V * 1000."""
    i = _shortlog_index(ts_str)
    value = 228000 + (i % 8) * 1000  # 228-235V range (*1000)
    return {"Value": value, "Usage": 0}


def gen_shortlog_water(ts_str):
    """Meter short-log for Water. Value=cumulative counter (litres)."""
    i = _shortlog_index(ts_str)
    value = 12000 + i * 50  # increasing counter
    return {"Value": value, "Usage": 0}


def gen_shortlog_kwh(ts_str):
    """Meter short-log for kWh. Value=cumulative Wh counter."""
    i = _shortlog_index(ts_str)
    value = 12345000 + i * 500  # increasing counter
    return {"Value": value, "Usage": 150 + (i % 10) * 30}


def gen_shortlog_p1power(ts_str):
    """MultiMeter short-log for P1 Power. Values are cumulative counters."""
    i = _shortlog_index(ts_str)
    base1 = 100000 + i * 400
    base2 = 60000 + i * 250
    return {"Value1": base1, "Value2": base2, "Value3": 200 + (i % 8) * 50, "Value4": 300 + (i % 6) * 40,
            "Value5": 5000 + i * 30, "Value6": 3000 + i * 20}


def gen_shortlog_p1gas(ts_str):
    """Meter short-log for P1 Gas. Value=cumulative counter."""
    i = _shortlog_index(ts_str)
    value = 55000 + i * 100  # increasing counter
    return {"Value": value, "Usage": 0}


def gen_shortlog_custom(ts_str):
    """Percentage short-log for Custom sensor."""
    i = _shortlog_index(ts_str)
    value = 35.0 + (i % 15) * 1.5  # 35-56.5 range
    return {"Percentage": round(value, 1)}


def gen_shortlog_temperature(ts_str):
    """Temperature short-log."""
    i = _shortlog_index(ts_str)
    temp = 15.0 + (i % 12) * 0.8  # 15.0-23.8 range
    chill = temp - 2.5
    humidity = 55 + (i % 20)
    dewpoint = temp - 6.0
    return {"Temperature": round(temp, 1), "Chill": round(chill, 1),
            "Humidity": humidity, "Barometer": 1010 + (i % 8),
            "DewPoint": round(dewpoint, 1), "SetPoint": 0.0}


SHORTLOG_GENERATORS = {
    "Test Usage Sensor":       gen_shortlog_usage,
    "Test Weight Sensor":      gen_shortlog_weight,
    "Test Rain Sensor":        gen_shortlog_rain,
    "Test Wind Sensor":        gen_shortlog_wind,
    "Test Voltage Sensor":     gen_shortlog_voltage,
    "Test Water Sensor":       gen_shortlog_water,
    "Test kWh Sensor":         gen_shortlog_kwh,
    "Test P1 Power Sensor":    gen_shortlog_p1power,
    "Test P1 Gas Sensor":      gen_shortlog_p1gas,
    "Test Custom Sensor":      gen_shortlog_custom,
    "Test Temperature Sensor": gen_shortlog_temperature,
}

# ---------------------------------------------------------------------------
# SQL insert helpers per table type
# ---------------------------------------------------------------------------

CALENDAR_SQL = {
    "MultiMeter_Calendar": """
        INSERT OR REPLACE INTO MultiMeter_Calendar
            (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6, Date)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """,
    "Rain_Calendar": """
        INSERT OR REPLACE INTO Rain_Calendar
            (DeviceRowID, Total, Rate, Date)
        VALUES (?, ?, ?, ?)
    """,
    "Wind_Calendar": """
        INSERT OR REPLACE INTO Wind_Calendar
            (DeviceRowID, Direction, Speed_Min, Speed_Max, Gust_Min, Gust_Max, Date)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    """,
    "Percentage_Calendar": """
        INSERT OR REPLACE INTO Percentage_Calendar
            (DeviceRowID, Percentage_Min, Percentage_Max, Percentage_Avg, Date)
        VALUES (?, ?, ?, ?, ?)
    """,
    "Meter_Calendar": """
        INSERT OR REPLACE INTO Meter_Calendar
            (DeviceRowID, Value, Counter, Date)
        VALUES (?, ?, ?, ?)
    """,
    "Temperature_Calendar": """
        INSERT OR REPLACE INTO Temperature_Calendar
            (DeviceRowID, Temp_Min, Temp_Max, Temp_Avg, Chill_Min, Chill_Max,
             Humidity, Barometer, DewPoint, SetPoint_Min, SetPoint_Max, SetPoint_Avg, Date)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """,
}

SHORTLOG_SQL = {
    "MultiMeter": """
        INSERT INTO MultiMeter
            (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6, Date)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """,
    "Rain": """
        INSERT INTO Rain
            (DeviceRowID, Total, Rate, Date)
        VALUES (?, ?, ?, ?)
    """,
    "Wind": """
        INSERT INTO Wind
            (DeviceRowID, Direction, Speed, Gust, Date)
        VALUES (?, ?, ?, ?, ?)
    """,
    "Percentage": """
        INSERT INTO Percentage
            (DeviceRowID, Percentage, Date)
        VALUES (?, ?, ?)
    """,
    "Meter": """
        INSERT INTO Meter
            (DeviceRowID, Value, Usage, Date)
        VALUES (?, ?, ?, ?)
    """,
    "Temperature": """
        INSERT INTO Temperature
            (DeviceRowID, Temperature, Chill, Humidity, Barometer, DewPoint, SetPoint, Date)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """,
}

# Column ordering for building parameter tuples
CALENDAR_COLS = {
    "MultiMeter_Calendar": ["Value1", "Value2", "Value3", "Value4", "Value5", "Value6"],
    "Rain_Calendar":       ["Total", "Rate"],
    "Wind_Calendar":       ["Direction", "Speed_Min", "Speed_Max", "Gust_Min", "Gust_Max"],
    "Percentage_Calendar": ["Percentage_Min", "Percentage_Max", "Percentage_Avg"],
    "Meter_Calendar":      ["Value", "Counter"],
    "Temperature_Calendar": ["Temp_Min", "Temp_Max", "Temp_Avg", "Chill_Min", "Chill_Max",
                             "Humidity", "Barometer", "DewPoint", "SetPoint_Min", "SetPoint_Max", "SetPoint_Avg"],
}

SHORTLOG_COLS = {
    "MultiMeter": ["Value1", "Value2", "Value3", "Value4", "Value5", "Value6"],
    "Rain":       ["Total", "Rate"],
    "Wind":       ["Direction", "Speed", "Gust"],
    "Percentage": ["Percentage"],
    "Meter":      ["Value", "Usage"],
    "Temperature": ["Temperature", "Chill", "Humidity", "Barometer", "DewPoint", "SetPoint"],
}


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    db_path = os.path.normpath(DB_PATH)
    if not os.path.exists(db_path):
        print(f"ERROR: Database not found at {db_path}")
        sys.exit(1)

    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()

    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # ------------------------------------------------------------------
    # 1. Upsert DeviceStatus rows
    #    DeviceStatus has no UNIQUE constraint, so we must check existence
    #    using the composite key (HardwareID, DeviceID, Unit, Type, SubType).
    # ------------------------------------------------------------------
    print("Inserting / updating DeviceStatus rows...")

    select_device_sql = """
        SELECT ID FROM DeviceStatus
        WHERE HardwareID=? AND DeviceID=? AND Unit=? AND Type=? AND SubType=?
    """
    update_device_sql = """
        UPDATE DeviceStatus
        SET SwitchType=?, SignalLevel=12, BatteryLevel=255, nValue=0,
            sValue=?, Name=?, Used=1, Options=?
        WHERE ID=?
    """
    insert_device_sql = """
        INSERT INTO DeviceStatus
            (HardwareID, DeviceID, Unit, Type, SubType, SwitchType,
             SignalLevel, BatteryLevel, nValue, sValue, Name, Used, Options)
        VALUES
            (?, ?, ?, ?, ?, ?,
             12, 255, 0, ?, ?, 1, ?)
    """

    for dev in DEVICES:
        cur.execute(select_device_sql, (
            HARDWARE_ID, dev["device_id"], dev["unit"], dev["type"], dev["subtype"],
        ))
        row = cur.fetchone()
        if row:
            cur.execute(update_device_sql, (
                dev["switch_type"], dev["s_value"], dev["name"], dev["options"], row["ID"],
            ))
            print(f"  Updated: {dev['name']} (ID={row['ID']})")
        else:
            cur.execute(insert_device_sql, (
                HARDWARE_ID, dev["device_id"], dev["unit"], dev["type"], dev["subtype"],
                dev["switch_type"], dev["s_value"], dev["name"], dev["options"],
            ))
            print(f"  Inserted: {dev['name']}")

    conn.commit()

    # ------------------------------------------------------------------
    # 2. Query back the assigned IDs
    # ------------------------------------------------------------------
    print("\nQuerying device IDs...")
    name_to_id = {}
    for dev in DEVICES:
        cur.execute(
            "SELECT ID FROM DeviceStatus WHERE HardwareID=? AND DeviceID=? AND Unit=?",
            (HARDWARE_ID, dev["device_id"], dev["unit"]),
        )
        row = cur.fetchone()
        if row is None:
            print(f"ERROR: Could not find ID for {dev['name']}")
            conn.close()
            sys.exit(1)
        name_to_id[dev["name"]] = row["ID"]
        print(f"  {dev['name']} -> ID={row['ID']}")

    # ------------------------------------------------------------------
    # 3. Delete existing data for these devices (clean slate)
    #    Delete from ALL possible tables to handle table reassignments.
    # ------------------------------------------------------------------
    ALL_CALENDAR_TABLES = ["MultiMeter_Calendar", "Rain_Calendar", "Wind_Calendar",
                           "Percentage_Calendar", "Meter_Calendar", "Temperature_Calendar"]
    ALL_SHORTLOG_TABLES = ["MultiMeter", "Rain", "Wind", "Percentage", "Meter", "Temperature"]

    print("\nCleaning existing data for test devices...")
    for dev in DEVICES:
        dev_id = name_to_id[dev["name"]]
        total_deleted = 0

        for table in ALL_CALENDAR_TABLES + ALL_SHORTLOG_TABLES:
            cur.execute(f"DELETE FROM [{table}] WHERE DeviceRowID=?", (dev_id,))
            total_deleted += cur.rowcount

        print(f"  {dev['name']}: deleted {total_deleted} total rows")

    conn.commit()

    # ------------------------------------------------------------------
    # 4. Insert calendar data (24 monthly + ~60 daily entries)
    # ------------------------------------------------------------------
    print(f"\nInserting calendar data ({len(ALL_CALENDAR_DATES)} dates per device)...")

    for dev in DEVICES:
        dev_id = name_to_id[dev["name"]]
        cal_table = dev["calendar"]
        gen_func = CALENDAR_GENERATORS[dev["name"]]
        sql = CALENDAR_SQL[cal_table]
        cols = CALENDAR_COLS[cal_table]

        count = 0
        for m in ALL_CALENDAR_DATES:
            vals = gen_func(m)
            params = tuple([dev_id] + [vals[c] for c in cols] + [m.strftime("%Y-%m-%d")])
            cur.execute(sql, params)
            count += 1

        print(f"  {dev['name']}: {count} rows -> {cal_table}")

    conn.commit()

    # ------------------------------------------------------------------
    # 5. Insert short-log data (last 7 days, 5 entries/day)
    # ------------------------------------------------------------------
    print(f"\nInserting short-log data ({len(SHORTLOG_TIMES)} entries per device)...")

    for dev in DEVICES:
        dev_id = name_to_id[dev["name"]]
        log_table = dev["shortlog"]
        gen_func = SHORTLOG_GENERATORS[dev["name"]]
        sql = SHORTLOG_SQL[log_table]
        cols = SHORTLOG_COLS[log_table]

        count = 0
        for ts in SHORTLOG_TIMES:
            vals = gen_func(ts)
            params = tuple([dev_id] + [vals[c] for c in cols] + [ts])
            cur.execute(sql, params)
            count += 1

        print(f"  {dev['name']}: {count} rows -> {log_table}")

    conn.commit()

    # ------------------------------------------------------------------
    # 6. Summary
    # ------------------------------------------------------------------
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Database    : {db_path}")
    print(f"Hardware ID : {HARDWARE_ID}")
    print(f"Date range  : {MONTHS[0].strftime('%Y-%m')} to {MONTHS[-1].strftime('%Y-%m')} ({len(MONTHS)} months + {len(DAILY_DATES)} daily)")
    print(f"Short-log   : last {SHORTLOG_DAYS} days, {SHORTLOG_PER_DAY} entries/day ({len(SHORTLOG_TIMES)} total)")
    print()

    for dev in DEVICES:
        dev_id = name_to_id[dev["name"]]
        print(f"  {dev['name']}")
        print(f"    DeviceStatus.ID = {dev_id}")
        print(f"    Type=0x{dev['type']:02X} ({dev['type']}), SubType=0x{dev['subtype']:02X}")
        print(f"    Calendar: {dev['calendar']}, ShortLog: {dev['shortlog']}")

    print()
    print("Test data insertion complete.")

    conn.close()


if __name__ == "__main__":
    main()
