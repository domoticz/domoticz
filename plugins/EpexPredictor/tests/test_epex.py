"""Unit tests for the EpexPredictor plugin helpers. Run: python -m unittest discover -s plugins/EpexPredictor/tests"""
import io
import json
import os
import sys
import unittest
import urllib.error
from datetime import date, datetime, timedelta, timezone

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

import tempfile  # noqa: E402

import epex_api  # noqa: E402
import epex_ledger  # noqa: E402
import epex_logic  # noqa: E402

CET = timezone(timedelta(hours=1))
T0 = datetime(2026, 11, 4, 0, 0, tzinfo=CET)


def quarter_slots(prices, start=T0, seconds=900):
    return [(start + timedelta(seconds=seconds * i), p) for i, p in enumerate(prices)]


class FakeResponse:
    def __init__(self, payload):
        self._raw = payload if isinstance(payload, bytes) else json.dumps(payload).encode()

    def read(self):
        return self._raw

    def __enter__(self):
        return self

    def __exit__(self, *a):
        return False


class UrlTests(unittest.TestCase):
    def test_normalize(self):
        n = epex_api.normalize_base_url
        self.assertEqual(n(""), "https://epexpredictor.batzill.com")
        self.assertEqual(n("epexpredictor.batzill.com"), "https://epexpredictor.batzill.com")
        self.assertEqual(n("192.168.1.20:8000"), "http://192.168.1.20:8000")
        self.assertEqual(n("192.168.1.20"), "http://192.168.1.20")
        self.assertEqual(n("epexpredictor:8000"), "http://epexpredictor:8000")
        self.assertEqual(n("localhost"), "http://localhost")
        self.assertEqual(n("https://example.org/epex/"), "https://example.org/epex")
        self.assertEqual(n("  HTTP://nas.local:8000/ "), "HTTP://nas.local:8000")
        self.assertEqual(n("[::1]:8000"), "http://[::1]:8000")


class ParseTests(unittest.TestCase):
    def test_parse_sorts_dedups_and_skips_bad_rows(self):
        payload = {"knownUntil": "2026-11-04T23:45:00+01:00", "prices": [
            {"startsAt": "2026-11-04T00:15:00+01:00", "total": 2.0},
            {"startsAt": "2026-11-04T00:00:00+01:00", "total": 1.0},
            {"startsAt": "2026-11-04T00:00:00+01:00", "total": 1.5},
            {"startsAt": "garbage", "total": 3},
            {"startsAt": "2026-11-04T00:30:00+01:00", "total": None},
            {"startsAt": "2026-11-04T00:45:00", "total": 4},
        ]}
        slots, known = epex_logic.parse_slots(payload)
        self.assertEqual([p for _, p in slots], [1.5, 2.0])
        self.assertEqual(known, datetime(2026, 11, 4, 23, 45, tzinfo=CET))

    def test_parse_skips_non_finite_and_non_dict_rows(self):
        payload = {"prices": [
            {"startsAt": "2026-11-04T00:00:00+01:00", "total": float("nan")},
            {"startsAt": "2026-11-04T00:15:00+01:00", "total": float("inf")},
            "garbage", 5,
            {"startsAt": "2026-11-04T00:30:00+01:00", "total": 2.5},
        ]}
        slots, _ = epex_logic.parse_slots(payload)
        self.assertEqual([p for _, p in slots], [2.5])

    def test_slot_seconds(self):
        self.assertEqual(epex_logic.slot_seconds(quarter_slots([1, 2, 3])), 900)
        self.assertEqual(epex_logic.slot_seconds(quarter_slots([1, 2, 3], seconds=3600)), 3600)
        self.assertEqual(epex_logic.slot_seconds([]), 3600)


class StatsTests(unittest.TestCase):
    def test_current_index(self):
        s = quarter_slots([1, 2, 3, 4])
        self.assertEqual(epex_logic.current_index(s, T0 + timedelta(minutes=20)), 1)
        self.assertEqual(epex_logic.current_index(s, T0), 0)
        self.assertEqual(epex_logic.current_index(s, T0 + timedelta(minutes=59)), 3)
        self.assertIsNone(epex_logic.current_index(s, T0 + timedelta(hours=1)))
        self.assertIsNone(epex_logic.current_index(s, T0 - timedelta(minutes=1)))
        self.assertIsNone(epex_logic.current_index([], T0))

    def test_day_stats_and_completeness(self):
        s = quarter_slots([float(i % 10) for i in range(96)])
        st = epex_logic.day_stats(s, date(2026, 11, 4))
        self.assertTrue(st["complete"])
        self.assertEqual((st["min"], st["max"], st["count"]), (0.0, 9.0, 96))
        self.assertIsNone(epex_logic.day_stats(s, date(2026, 11, 5)))
        partial = epex_logic.day_stats(s[40:], date(2026, 11, 4))
        self.assertFalse(partial["complete"])


class DstTests(unittest.TestCase):
    def test_spring_day_has_92_slots_and_is_complete(self):
        cet, cest = timezone(timedelta(hours=1)), timezone(timedelta(hours=2))
        start = datetime(2027, 3, 28, 0, 0, tzinfo=cet)
        s = [(start + timedelta(minutes=15 * i), 1.0) for i in range(8)]  # 00:00 to 02:00 CET
        after = datetime(2027, 3, 28, 3, 0, tzinfo=cest)
        s += [(after + timedelta(minutes=15 * i), 1.0) for i in range(84)]  # 03:00 CEST to midnight
        st = epex_logic.day_stats(s, date(2027, 3, 28))
        self.assertEqual(st["count"], 92)
        self.assertTrue(st["complete"])

    def test_autumn_day_has_100_slots_and_is_complete(self):
        cest, cet = timezone(timedelta(hours=2)), timezone(timedelta(hours=1))
        start = datetime(2026, 10, 25, 0, 0, tzinfo=cest)
        s = [(start + timedelta(minutes=15 * i), 1.0) for i in range(12)]  # 00:00 to 03:00 CEST
        later = datetime(2026, 10, 25, 2, 0, tzinfo=cet)
        s += [(later + timedelta(minutes=15 * i), 1.0) for i in range(88)]  # 02:00 CET to midnight
        st = epex_logic.day_stats(s, date(2026, 10, 25))
        self.assertEqual(st["count"], 100)
        self.assertTrue(st["complete"])
        self.assertFalse(epex_logic.day_stats(s[:99], date(2026, 10, 25))["complete"])


class CheapestTests(unittest.TestCase):
    def test_finds_cheapest_window(self):
        prices = [10] * 8 + [1, 1, 1, 1] + [10] * 20
        s = quarter_slots(prices)
        w = epex_logic.cheapest_window(s, T0, 1)
        self.assertEqual(w["start"], T0 + timedelta(hours=2))
        self.assertEqual(w["avg"], 1)
        self.assertFalse(w["active"])

    def test_running_window_is_stable(self):
        prices = [10] * 8 + [1, 1, 1, 1] + [10] * 20
        s = quarter_slots(prices)
        inside = T0 + timedelta(hours=2, minutes=20)
        w = epex_logic.cheapest_window(s, inside, 1)
        self.assertEqual(w["start"], T0 + timedelta(hours=2))
        self.assertTrue(w["active"])
        after = T0 + timedelta(hours=3)
        self.assertNotEqual(epex_logic.cheapest_window(s, after, 1)["start"], T0 + timedelta(hours=2))

    def test_gap_and_short_data(self):
        s = quarter_slots([1, 1]) + quarter_slots([1, 1], start=T0 + timedelta(hours=5))
        self.assertIsNone(epex_logic.cheapest_window(s, T0, 1))
        self.assertIsNone(epex_logic.cheapest_window([], T0, 1))
        self.assertIsNone(epex_logic.cheapest_window(quarter_slots([1, 2]), T0, 0))

    def test_horizon_limits_the_search(self):
        s = quarter_slots([5] * 96 + [0] * 8)  # a very cheap period only after the first 24 h
        w = epex_logic.cheapest_window(s, T0, 1, 24)
        self.assertEqual(w["avg"], 5)


class LevelTests(unittest.TestCase):
    def test_levels(self):
        s = quarter_slots(list(range(100)))
        self.assertEqual(epex_logic.price_level(s, T0, 0)[0], "Very cheap")
        self.assertEqual(epex_logic.price_level(s, T0, 30)[0], "Cheap")
        self.assertEqual(epex_logic.price_level(s, T0, 50)[0], "Normal")
        self.assertEqual(epex_logic.price_level(s, T0, 75)[0], "Expensive")
        self.assertEqual(epex_logic.price_level(s, T0, 99)[0], "Very expensive")
        self.assertIsNone(epex_logic.price_level([], T0, 1))

    def test_flat_prices_are_normal(self):
        self.assertEqual(epex_logic.price_level(quarter_slots([3] * 10), T0, 3)[0], "Normal")

    def test_hourly_average(self):
        h = epex_logic.hourly_average(quarter_slots([1, 2, 3, 4, 10, 10, 10, 10]))
        self.assertEqual([p for _, p in h], [2.5, 10])


class ClientTests(unittest.TestCase):
    def test_request_url_and_params(self):
        seen = []

        def opener(req, timeout=0):
            seen.append(req.full_url)
            return FakeResponse({"prices": [], "knownUntil": "2026-11-04T23:45:00+01:00"})

        c = epex_api.EpexClient("192.168.1.20:8000", opener=opener)
        c.get_prices(region="NL", unit="EUR_PER_KWH", surcharge=1.5, tax_percent=21, hourly=True, timezone="Europe/Amsterdam")
        url = seen[0]
        self.assertTrue(url.startswith("http://192.168.1.20:8000/prices?"))
        for part in ("region=NL", "unit=EUR_PER_KWH", "surcharge=1.5", "taxPercent=21", "hourly=true", "timezone=Europe%2FAmsterdam", "hours=-1"):
            self.assertIn(part, url)
        self.assertNotIn("startTs", url)

    def test_http_error(self):
        def opener(req, timeout=0):
            raise urllib.error.HTTPError(req.full_url, 422, "x", {}, io.BytesIO(b'{"detail": "bad region"}'))

        with self.assertRaises(epex_api.EpexError) as ctx:
            epex_api.EpexClient("h.example", opener=opener).get_prices()
        self.assertEqual(ctx.exception.status, 422)
        self.assertIn("bad region", str(ctx.exception))

    def test_network_error(self):
        def opener(req, timeout=0):
            raise urllib.error.URLError(ConnectionRefusedError())

        with self.assertRaises(epex_api.EpexError) as ctx:
            epex_api.EpexClient("localhost:9", opener=opener).get_prices()
        self.assertEqual(ctx.exception.status, 0)
        self.assertIn("http://localhost:9", str(ctx.exception))

    def test_get_info(self):
        doc = {"info": {"title": "EPEX day-ahead prediction API", "version": "0.1.0"},
               "components": {"schemas": {"PriceRegionName": {"enum": ["DE", "NL"]}}}}
        info = epex_api.EpexClient("h.example", opener=lambda req, timeout=0: FakeResponse(doc)).get_info()
        self.assertEqual(info, {"title": "EPEX day-ahead prediction API", "version": "0.1.0", "regions": ["DE", "NL"]})

    def test_get_info_is_optional(self):
        def opener(req, timeout=0):
            raise urllib.error.HTTPError(req.full_url, 404, "x", {}, io.BytesIO(b"{}"))

        self.assertEqual(epex_api.EpexClient("h.example", opener=opener).get_info(), {})
        self.assertEqual(epex_api.EpexClient("h.example", opener=lambda req, timeout=0: FakeResponse([1])).get_info(), {})

    def test_not_an_epex_server(self):
        for body in (b"<html>nope</html>", json.dumps({"hello": 1}).encode()):
            with self.assertRaises(epex_api.EpexError):
                epex_api.EpexClient("h.example", opener=lambda req, timeout=0, b=body: FakeResponse(b)).get_prices()


class LedgerTests(unittest.TestCase):
    def test_forecast_is_overwritten_until_published_then_actual_is_filled(self):
        led = epex_ledger.Ledger("sig")
        known = T0 + timedelta(minutes=15)
        led.update(quarter_slots([1.0, 1.0, 5.0, 6.0]), known, 100)  # slots 2 and 3 are forecast
        led.update(quarter_slots([1.0, 1.0, 7.0, 6.0]), known, 200)  # newer forecast for slot 2
        ts2 = int((T0 + timedelta(minutes=30)).timestamp())
        self.assertEqual((led.rows[ts2]["fc"], led.rows[ts2]["issued"]), (7.0, 200))
        led.update(quarter_slots([1.0, 1.0, 9.0, 9.0]), T0 + timedelta(minutes=45), 300)  # published now
        self.assertEqual(led.rows[ts2]["act"], 9.0)
        self.assertEqual(led.rows[ts2]["fc"], 7.0)
        led.update(quarter_slots([1.0, 1.0, 8.0, 8.0]), T0 + timedelta(minutes=45), 400)  # actual is final
        self.assertEqual(led.rows[ts2]["act"], 9.0)
        self.assertEqual([p[1:] for p in led.pairs()], [(7.0, 9.0), (6.0, 9.0)])

    def test_slots_published_without_a_forecast_are_not_compared(self):
        led = epex_ledger.Ledger("sig")
        led.update(quarter_slots([1.0, 2.0]), T0 + timedelta(minutes=15), 100)
        self.assertEqual(led.pairs(), [])

    def test_prune_and_pair_range(self):
        led = epex_ledger.Ledger("sig")
        led.update(quarter_slots([1.0, 2.0]), T0 - timedelta(minutes=15), int(T0.timestamp()))
        self.assertEqual(len(led.rows), 2)
        self.assertTrue(led.prune(int(T0.timestamp()) + 31 * 86400))
        self.assertEqual(led.rows, {})

    def test_save_load_and_signature_reset(self):
        led = epex_ledger.Ledger("a")
        led.update(quarter_slots([1.0, 2.0]), T0, 100)
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "sub", "l.json")
            led.save(path)
            self.assertEqual(len(epex_ledger.Ledger.load(path, "a").rows), 1)
            self.assertEqual(epex_ledger.Ledger.load(path, "b").rows, {})  # other settings: start again
            self.assertEqual(epex_ledger.Ledger.load(os.path.join(tmp, "missing.json"), "a").rows, {})
            with open(path, "w") as fh:
                fh.write("garbage")
            self.assertEqual(epex_ledger.Ledger.load(path, "a").rows, {})


class LedgerEdgeTests(unittest.TestCase):
    def test_slot_starting_at_known_until_is_published(self):
        # knownUntil is the start of the last published slot: that slot gets its actual, the next stays a forecast
        led = epex_ledger.Ledger("s")
        led.update(quarter_slots([1.0, 2.0, 3.0]), T0 - timedelta(minutes=15), 10)  # all forecast
        known = T0 + timedelta(minutes=15)
        led.update(quarter_slots([1.5, 2.5, 3.5]), known, 20)
        r = led.rows
        self.assertEqual(r[int(known.timestamp())]["act"], 2.5)
        self.assertIsNone(r[int((known + timedelta(minutes=15)).timestamp())]["act"])

    def test_missing_known_until_never_overwrites_a_forecast(self):
        led = epex_ledger.Ledger("s")
        led.update(quarter_slots([5.0, 6.0]), None, 10)  # nothing known: no row is created either
        self.assertEqual(led.rows, {})
        known = T0 + timedelta(minutes=15)
        led.update(quarter_slots([1.0, 2.0, 3.0]), T0 - timedelta(minutes=15), 20)  # both slots are a forecast
        led.update(quarter_slots([8.0, 9.0, 3.0]), None, 30)  # real prices, but the server forgot knownUntil
        led.update(quarter_slots([8.0, 9.0, 3.0]), known, 40)  # knownUntil is back
        self.assertEqual([p[1:] for p in led.pairs()], [(1.0, 8.0), (2.0, 9.0)])

    def test_corrupt_saved_rows(self):
        for doc in ({"signature": "s", "rows": 5}, {"signature": "s", "rows": [[1, float("nan"), 0, 1.0], [2, 1.0, 0, float("inf")], "x", [3, 1.0, 0, None]]}, [1], None):
            led = epex_ledger.Ledger.from_json(doc, "s")
            self.assertLessEqual(len(led.rows), 1)
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "l.json")
            with open(path, "w") as fh:
                fh.write("null")
            self.assertEqual(epex_ledger.Ledger.load(path, "s").rows, {})


class RealZoneDstTests(unittest.TestCase):
    """The plugin groups by day with a ZoneInfo time zone, whose datetimes subtract by wall clock."""

    def setUp(self):
        try:
            from zoneinfo import ZoneInfo
            self.tz = ZoneInfo("Europe/Amsterdam")
        except Exception:  # noqa: BLE001
            self.skipTest("no time zone database")

    def day_pairs(self, start, count):
        ts0 = int(start.timestamp())
        pairs = []
        for i in range(count):
            act = 10.0 if not 8 <= i < 12 else 1.0
            pairs.append((ts0 + 900 * i, act, act))
        return pairs

    def test_spring_day_is_complete_with_regret(self):
        start = datetime(2027, 3, 28, 0, 0, tzinfo=self.tz)
        r = epex_logic.accuracy(self.day_pairs(start, 92), self.tz, 1)
        self.assertEqual(r["days"][0]["n"], 92)
        self.assertAlmostEqual(r["days"][0]["regret"], 0.0)

    def test_autumn_day_is_complete_with_regret(self):
        start = datetime(2026, 10, 25, 0, 0, tzinfo=self.tz)
        r = epex_logic.accuracy(self.day_pairs(start, 100), self.tz, 1)
        self.assertEqual(r["days"][0]["n"], 100)
        self.assertAlmostEqual(r["days"][0]["regret"], 0.0)

    def test_window_across_the_spring_gap(self):
        # the cheapest 2 hours are 01:00 to 03:00 local, which crosses the missing hour
        start = datetime(2027, 3, 28, 0, 0, tzinfo=self.tz)
        pairs = self.day_pairs(start, 92)
        pairs = [(ts, 10.0, 10.0) for ts, _, _ in pairs]
        pairs = [(ts, 1.0, 1.0) if 4 <= i < 12 else (ts, 10.0, 10.0) for i, (ts, _, _) in enumerate(pairs)]
        r = epex_logic.accuracy(pairs, self.tz, 2)
        self.assertAlmostEqual(r["days"][0]["regret"], 0.0)


class AccuracyTests(unittest.TestCase):
    def pairs_for(self, fc, act):
        return [(int((T0 + timedelta(minutes=15 * i)).timestamp()), f, a) for i, (f, a) in enumerate(zip(fc, act))]

    def test_error_statistics(self):
        r = epex_logic.accuracy(self.pairs_for([10, 10, 10, 10], [8, 12, 10, 14]), CET, 1)
        s = r["summary"]
        self.assertEqual(s["n"], 4)
        self.assertAlmostEqual(s["mae"], 2.0)    # errors 2, -2, 0, -4
        self.assertAlmostEqual(s["bias"], -1.0)
        self.assertAlmostEqual(s["rmse"], (6.0) ** 0.5)
        self.assertEqual(len(r["days"]), 1)
        self.assertIsNone(r["days"][0]["regret"])  # a partial day has no regret

    def test_empty(self):
        r = epex_logic.accuracy([], CET, 3)
        self.assertEqual((r["summary"]["n"], r["summary"]["mae"], r["days"]), (0, None, []))

    def test_regret_of_a_complete_day(self):
        # the real cheapest hour is 02:00 (price 1), the forecast thought 05:00 was cheapest, where the real price is 4
        act = [10.0] * 96
        fc = [10.0] * 96
        for i in range(8, 12):
            act[i] = 1.0
        for i in range(20, 24):
            fc[i] = 0.5
            act[i] = 4.0
        r = epex_logic.accuracy(self.pairs_for(fc, act), CET, 1)
        self.assertAlmostEqual(r["days"][0]["regret"], 3.0)
        # a perfect forecast has no regret
        r = epex_logic.accuracy(self.pairs_for(act, act), CET, 1)
        self.assertAlmostEqual(r["days"][0]["regret"], 0.0)
        self.assertAlmostEqual(r["summary"]["mae"], 0.0)


if __name__ == "__main__":
    unittest.main()
