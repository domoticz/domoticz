"""Pure helpers for the EpexPredictor plugin: price slots, daily statistics and the cheapest window.

No Domoticz imports, so everything here is unit testable. A slot is a tuple (start: aware datetime, price: float).
"""
import math
from datetime import datetime, timedelta

LEVELS = ("Very cheap", "Cheap", "Normal", "Expensive", "Very expensive")
# Domoticz alert levels: 1 green, 2 yellow, 3 orange, 4 red
LEVEL_ALERT = {"Very cheap": 1, "Cheap": 1, "Normal": 2, "Expensive": 3, "Very expensive": 4}


def parse_iso(value):
    """Parse an ISO 8601 timestamp into an aware datetime, None when it cannot be parsed."""
    if not value:
        return None
    text = str(value).strip()
    if text.endswith("Z"):
        text = text[:-1] + "+00:00"
    try:
        dt = datetime.fromisoformat(text)
    except ValueError:
        return None
    return dt if dt.tzinfo else None


def parse_slots(payload):
    """Return (slots sorted by time with duplicates removed, known_until) from a /prices answer."""
    seen = {}
    for row in payload.get("prices") or []:
        if not isinstance(row, dict):
            continue
        start = parse_iso(row.get("startsAt"))
        total = row.get("total")
        if start is None or not isinstance(total, (int, float)) or isinstance(total, bool) or not math.isfinite(total):
            continue
        seen[start] = float(total)
    slots = sorted(seen.items())
    return slots, parse_iso(payload.get("knownUntil"))


def slot_seconds(slots):
    """Length of one price slot, the smallest distance between two neighbours (900 for quarter hours)."""
    best = 0
    for a, b in zip(slots, slots[1:]):
        d = int((b[0] - a[0]).total_seconds())
        if d > 0 and (best == 0 or d < best):
            best = d
    return best or 3600


def current_index(slots, now):
    """Index of the slot that contains now, None when now is outside the data."""
    if not slots:
        return None
    length = timedelta(seconds=slot_seconds(slots))
    lo, hi, found = 0, len(slots) - 1, None
    while lo <= hi:
        mid = (lo + hi) // 2
        if slots[mid][0] <= now:
            found = mid
            lo = mid + 1
        else:
            hi = mid - 1
    if found is None or now >= slots[found][0] + length:
        return None
    return found


def day_slots(slots, day):
    """Slots whose local start (in the time zone of the data) falls on the given date."""
    return [s for s in slots if s[0].date() == day]


def day_stats(slots, day):
    """{avg, min, max, count, complete} for one local day, None when there is no data. complete means 00:00 to 24:00."""
    rows = day_slots(slots, day)
    if not rows:
        return None
    prices = [p for _, p in rows]
    complete = False
    if len(slots) > 1:
        # a day has 23, 24 or 25 hours: it is complete when it runs from 00:00 to the next midnight without gaps
        length = timedelta(seconds=slot_seconds(slots))
        first, last = rows[0][0], rows[-1][0]
        span = (last + length - first).total_seconds()
        complete = (first.hour == 0 and first.minute == 0 and (last + length).date() > day
                    and len(rows) >= int(span // length.total_seconds()))
    return {"avg": sum(prices) / len(prices), "min": min(prices), "max": max(prices), "count": len(prices),
            "complete": complete}


def cheapest_window(slots, now, hours, horizon_hours=24):
    """The cheapest run of consecutive slots lasting `hours`, searched over the next `horizon_hours`.

    A window that is already running still counts, so the answer stays the same while it is in progress.
    Returns {start, end, avg, active} or None when there is not enough data.
    """
    if not slots or hours <= 0:
        return None
    length = slot_seconds(slots)
    n = max(1, int(round(hours * 3600 / length)))
    step = timedelta(seconds=length)
    horizon_end = now + timedelta(hours=horizon_hours)
    best = None
    for i in range(0, len(slots) - n + 1):
        start = slots[i][0]
        end = slots[i + n - 1][0] + step
        if end - start != step * n:
            continue  # a gap in the data, not a continuous window
        if end <= now or end > horizon_end:
            continue
        avg = sum(p for _, p in slots[i:i + n]) / n
        if best is None or avg < best["avg"] - 1e-9:
            best = {"start": start, "end": end, "avg": avg}
    if best:
        best["active"] = best["start"] <= now < best["end"]
    return best


def price_level(slots, now, price, horizon_hours=24):
    """(label, percentile) of a price against the slots of the next horizon, None without data."""
    window = [p for s, p in slots if now <= s < now + timedelta(hours=horizon_hours)]
    if not window:
        return None
    below = sum(1 for p in window if p < price)
    equal = sum(1 for p in window if p == price)
    pct = (below + equal / 2.0) / len(window) * 100.0
    for limit, label in ((15, LEVELS[0]), (40, LEVELS[1]), (60, LEVELS[2]), (85, LEVELS[3])):
        if pct <= limit:
            return label, pct
    return LEVELS[4], pct


def hourly_average(slots):
    """Collapse quarter hour slots into hour slots (mean per clock hour), for compact tables."""
    out = {}
    for start, price in slots:
        key = start.replace(minute=0, second=0, microsecond=0)
        out.setdefault(key, []).append(price)
    return [(k, sum(v) / len(v)) for k, v in sorted(out.items())]
