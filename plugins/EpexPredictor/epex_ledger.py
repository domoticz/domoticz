"""Forecast ledger: remembers what the forecast was before a price was published, to compare it with the real price.

The EpexPredictor server keeps no history of its forecasts, so the plugin records them itself. No Domoticz imports.
"""
import json
import math
import os

RETENTION_DAYS = 30


class Ledger:
    """rows maps the slot start (unix seconds) to {"fc": forecast, "issued": when the forecast was fetched, "act": real price}."""

    def __init__(self, signature=""):
        self.signature = signature
        self.rows = {}

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
        return changed

    def prune(self, now_ts, days=RETENTION_DAYS):
        old = [ts for ts in self.rows if ts < now_ts - days * 86400]
        for ts in old:
            del self.rows[ts]
        return bool(old)

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
                fc = float(fc)
                act = None if act is None else float(act)
                if not math.isfinite(fc) or (act is not None and not math.isfinite(act)):
                    continue
                ledger.rows[int(ts)] = {"fc": fc, "issued": issued, "act": act}
            except (TypeError, ValueError, OverflowError):
                continue
        return ledger

    def save(self, path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        tmp = path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as fh:
            json.dump(self.to_json(), fh, separators=(",", ":"), allow_nan=False)
        os.replace(tmp, path)

    @classmethod
    def load(cls, path, signature):
        try:
            with open(path, "r", encoding="utf-8") as fh:
                return cls.from_json(json.load(fh), signature)
        except (OSError, ValueError, TypeError):
            return cls(signature)
