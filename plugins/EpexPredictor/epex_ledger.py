"""Forecast ledger: remembers what the forecast was before a price was published, to compare it with the real price.

The EpexPredictor server keeps no history of its forecasts, so the plugin records them itself. No Domoticz imports.
"""
import gzip
import json
import math
import os
import time
import zlib

RETENTION_DAYS = 365
FUTURE_DAYS = 30  # forecasts reach about a week ahead, a row further away than this is corrupt
MAX_TS = 4000000000  # year 2096, later "time stamps" in a saved file are corrupt
CORRUPT = (gzip.BadGzipFile, zlib.error, EOFError, ValueError, TypeError)  # damaged content, not a failure to open


class Ledger:
    """rows maps the slot start (unix seconds) to {"fc": forecast, "issued": when the forecast was fetched, "act": real price}.

    revision goes up whenever the rows change, so results derived from the ledger can be cached.
    """

    def __init__(self, signature=""):
        self.signature = signature
        self.rows = {}
        self.revision = 0
        self.legacy_path = None  # plain file of an older version whose rows were loaded, removed after the first save
        self.unreadable = None  # a file with damaged content, moved aside before the first save
        self.locked = None  # a file that could not even be opened (in use by another program), read again before a save

    def update(self, slots, known_until, now_ts):
        """Record the forecast of unpublished slots and the real price of slots that got published. True when something changed."""
        if known_until is None:
            # without knownUntil a published price cannot be told from a forecast, so a real price must not overwrite a forecast
            return self.prune(now_ts)
        changed = False
        for start, price in slots:
            ts = int(start.timestamp())
            published = start <= known_until
            row = self.rows.get(ts)
            if not published:
                # the last forecast before publication is what a planner would have acted on, so keep overwriting
                if row is None:
                    row = self.rows[ts] = {"fc": price, "issued": now_ts, "act": None}
                    changed = True
                elif row["act"] is None and row["fc"] != price:
                    row["fc"], row["issued"] = price, now_ts
                    changed = True
            elif row is not None and row["act"] is None:
                row["act"] = price
                changed = True
        if self.prune(now_ts):
            changed = True
        if changed:
            self.revision += 1
        return changed

    def prune(self, now_ts, days=RETENTION_DAYS):
        old = [ts for ts in self.rows if ts < now_ts - days * 86400]
        for ts in old:
            del self.rows[ts]
        if old:
            self.revision += 1
        return bool(old)

    def drop_future(self, now_ts, days=FUTURE_DAYS):
        """Remove rows further ahead than any forecast reaches, they can only come from a damaged file."""
        far = [ts for ts in self.rows if ts > now_ts + days * 86400]
        for ts in far:
            del self.rows[ts]
        if far:
            self.revision += 1
        return bool(far)

    def pairs(self, from_ts=None, to_ts=None):
        """(ts, forecast, actual) of every slot that has both, in time order."""
        out = []
        for ts in sorted(self.rows):
            row = self.rows[ts]
            if row["fc"] is None or row["act"] is None:
                continue
            if (from_ts is not None and ts < from_ts) or (to_ts is not None and ts >= to_ts):
                continue
            out.append((ts, row["fc"], row["act"]))
        return out

    def to_json(self):
        return {"signature": self.signature,
                "rows": [[ts, r["fc"], r["issued"], r["act"]] for ts, r in sorted(self.rows.items())]}

    @classmethod
    def from_json(cls, doc, signature):
        """A ledger from saved data, an empty one when the data is unreadable or was made with other settings."""
        ledger = cls(signature)
        if not isinstance(doc, dict) or doc.get("signature") != signature:
            return ledger
        rows = doc.get("rows")
        for row in rows if isinstance(rows, list) else []:
            try:
                ts, fc, issued, act = row
                if not 0 < int(ts) < MAX_TS:
                    continue  # a corrupt time stamp would break the day grouping later (prune drops rows too far ahead)
                fc = float(fc)
                act = None if act is None else float(act)
                if not math.isfinite(fc) or (act is not None and not math.isfinite(act)):
                    continue
                # issued is only informative, but a NaN in it would make every later save fail
                issued = issued if isinstance(issued, (int, float)) and not isinstance(issued, bool) and math.isfinite(issued) else 0
                ledger.rows[int(ts)] = {"fc": fc, "issued": issued, "act": act}
            except (TypeError, ValueError, OverflowError):
                continue
        return ledger

    def save(self, path):
        """Write the ledger, gzip compressed when path ends in .gz.

        A plain file of an older version is removed only when its rows were loaded into this ledger, so a file that
        could not be used is left alone. A file that exists but could not be read (locked by another program for a
        moment, damaged) is moved aside to <name>.bad-<time> first instead of being overwritten; when that is not
        possible OSError is raised and nothing is written. A file that could not be opened at all (in use by another
        program at start up) is read again first and its rows are merged in, and while it still cannot be read
        OSError is raised, so a good history is never replaced by a ledger that started empty.
        """
        os.makedirs(os.path.dirname(path), exist_ok=True)
        if self.locked:
            self._take_over_locked()
        if self.unreadable and os.path.isfile(self.unreadable):
            os.replace(self.unreadable, "%s.bad-%d" % (self.unreadable, int(time.time())))
        self.unreadable = None
        tmp = path + ".tmp"
        try:
            if path.endswith(".gz"):
                fh = gzip.open(tmp, "wt", encoding="utf-8", compresslevel=6)  # nearly as small as 9, much faster
            else:
                fh = open(tmp, "w", encoding="utf-8")
            with fh:
                json.dump(self.to_json(), fh, separators=(",", ":"), allow_nan=False)
            os.replace(tmp, path)
        finally:
            if os.path.exists(tmp):
                try:
                    os.remove(tmp)
                except OSError:
                    pass
        if self.legacy_path and os.path.isfile(self.legacy_path):
            try:
                os.remove(self.legacy_path)
            except OSError:
                pass
        self.legacy_path = None

    def _take_over_locked(self):
        """Merge the rows of the file that was in use at start up. OSError while it still cannot be read."""
        again = Ledger.load(self.locked, self.signature)
        if again.locked:
            raise OSError("%s is in use by another program, not overwriting the history" % self.locked)
        for ts, row in again.rows.items():
            mine = self.rows.get(ts)
            if mine is None:
                self.rows[ts] = row
            elif mine["act"] is None and row["act"] is not None:
                mine["act"] = row["act"]
        if again.unreadable:
            self.unreadable = again.unreadable
        if again.legacy_path:
            self.legacy_path = again.legacy_path
        self.locked = None
        self.revision += 1

    @classmethod
    def load(cls, path, signature):
        """Read the ledger from path, or from the plain file an older version wrote next to a .gz path."""
        for candidate in (path, _legacy_path(path)):
            if not candidate or not os.path.isfile(candidate):
                continue
            opener = gzip.open if candidate.endswith(".gz") else open
            try:
                with opener(candidate, "rt", encoding="utf-8") as fh:
                    doc = json.load(fh)
            except CORRUPT:
                doc = None
            except OSError:
                # BadGzipFile is an OSError too and was handled above; this is a file that cannot be opened right now
                ledger = cls(signature)
                ledger.locked = path
                return ledger
            if not isinstance(doc, dict):
                # the file exists but is of no use now: start empty, but never overwrite it unseen
                ledger = cls(signature)
                ledger.unreadable = candidate
                return ledger
            ledger = cls.from_json(doc, signature)
            if candidate != path and ledger.rows:
                ledger.legacy_path = candidate
            return ledger
        return cls(signature)


def _legacy_path(path):
    return path[:-3] if path.endswith(".gz") else None
