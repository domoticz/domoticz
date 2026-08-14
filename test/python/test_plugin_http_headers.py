#!/usr/bin/env python3
"""
Regression test for the Python plugin HTTP response header parser
(CPluginProtocolHTTP::ExtractHeaders in hardware/plugins/PluginProtocols.cpp).

Covers GitHub issue #5341: a plugin using Protocol="HTTP" would stop working,
peg a CPU or lose responses when the peer sent header blocks that the parser
did not expect.  The cases below reproduce every failure mode:

  baseline     plain, well formed response                     (sanity check)
  nocolon      a header line without a ':'                     (threw std::out_of_range)
  emptyvalue   'X-Empty:' with no value at all                 (threw std::out_of_range)
  nospace      'X-NoSpace:value' without the usual space       (value lost its first char)
  splitheader  >8KB of headers split mid header line           (infinite loop, 100% CPU)
  splitblank   split inside the CRLF that terminates the block (response dispatched too early)

How it works
------------
A tiny scripted HTTP server hands out one case per connection.  The
HttpHeaderTest example plugin connects, sends a GET, and reports what it
parsed from the *previous* response in the query string of the next request.
No log scraping and no authentication are needed; a case that hangs the
parser simply never produces another connection.

By default the test starts its own Domoticz instance on a free port with a
throwaway database, so nothing touches an existing installation.

Run
---
    python test/python/test_plugin_http_headers.py

    DOMOTICZ_EXE=/path/to/domoticz    override executable autodetection
    --keep-log                        print the Domoticz log at the end
"""

import base64
import http.cookiejar
import json
import os
import queue
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
import urllib.parse
import urllib.request

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
PLUGIN_SOURCE = os.path.join(REPO_ROOT, "plugins", "examples", "HttpHeaderTest")
PLUGIN_TARGET = os.path.join(REPO_ROOT, "plugins", "HttpHeaderTest")
PLUGIN_KEY = "HttpHeaderTest"
HTYPE_PYTHON_PLUGIN = 94

CONNECT_TIMEOUT = 90  # seconds to wait for the plugin to show up at all
CASE_TIMEOUT = 30  # seconds to wait for each following case


def response(lines, body=b"OK"):
    return b"HTTP/1.1 200 OK\r\n" + b"".join(line + b"\r\n" for line in lines) + b"\r\n" + body


def split_at(data, position):
    return [data[:position], data[position:]]


def build_cases():
    cases = []

    cases.append(("baseline",
                  [response([b"X-Test: baseline", b"Content-Type: text/plain",
                             b"Content-Length: 2", b"Connection: close"])],
                  {}))

    cases.append(("nocolon",
                  [response([b"X-Test: nocolon", b"ThisLineHasNoColonAtAll",
                             b"Content-Type: text/plain", b"Content-Length: 2",
                             b"Connection: close"])],
                  {}))

    cases.append(("emptyvalue",
                  [response([b"X-Test: emptyvalue", b"X-Empty:",
                             b"Content-Length: 2", b"Connection: close"])],
                  {"empty": ""}))

    cases.append(("nospace",
                  [response([b"X-Test: nospace", b"X-NoSpace:novalue-space",
                             b"Content-Length: 2", b"Connection: close"])],
                  {"nospace": "novalue-space"}))

    filler = [b"X-Filler-%03d: %s" % (i, b"A" * 120) for i in range(80)]
    big = response([b"X-Test: splitheader"] + filler + [b"Content-Length: 2", b"Connection: close"])
    # Cut in the middle of a header line so the parser sees a partial one
    cases.append(("splitheader", split_at(big, big.index(b"X-Filler-040:") + 10), {}))

    blank = response([b"X-Test: splitblank", b"Content-Length: 2", b"Connection: close"])
    # Cut inside the CRLF CRLF that terminates the header block
    cases.append(("splitblank", split_at(blank, blank.index(b"\r\n\r\n") + 3), {}))

    return cases


DONE_RESPONSE = response([b"X-Test: done", b"Content-Length: 4", b"Connection: close"], b"DONE")


class CaseServer(threading.Thread):
    """Serves one scripted response per connection and collects the plugin's reports."""

    def __init__(self, cases):
        threading.Thread.__init__(self, daemon=True)
        self.cases = cases
        self.reports = queue.Queue()
        self.served = []
        self.running = True
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(("127.0.0.1", 0))
        self.socket.listen(5)
        self.port = self.socket.getsockname()[1]

    def stop(self):
        self.running = False
        try:
            self.socket.close()
        except OSError:
            pass

    def run(self):
        while self.running:
            try:
                connection, _ = self.socket.accept()
            except OSError:
                return
            try:
                self.handle(connection)
            except (OSError, socket.timeout):
                pass
            finally:
                try:
                    connection.close()
                except OSError:
                    pass

    def handle(self, connection):
        connection.settimeout(15)
        request = b""
        while b"\r\n\r\n" not in request:
            chunk = connection.recv(4096)
            if not chunk:
                break
            request += chunk

        index = len(self.served)
        self.served.append(index)
        self.reports.put((index, self.parse_report(request)))

        chunks = self.cases[index][1] if index < len(self.cases) else [DONE_RESPONSE]
        for position, chunk in enumerate(chunks):
            if position:
                time.sleep(0.4)  # force a separate read on the Domoticz side
            connection.sendall(chunk)

    @staticmethod
    def parse_report(request):
        try:
            target = request.split(b"\r\n", 1)[0].split(b" ")[1].decode("utf-8", "ignore")
        except IndexError:
            return "unparsable-request"
        query = urllib.parse.urlparse(target).query
        return urllib.parse.parse_qs(query).get("r", ["missing"])[0]


def parse_result(report):
    fields = {}
    for part in report.split("|"):
        if "=" in part:
            key, value = part.split("=", 1)
            fields[key] = value
    return fields


def free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def find_executable():
    override = os.environ.get("DOMOTICZ_EXE")
    if override:
        return override
    candidates = [os.path.join(REPO_ROOT, "msbuild", "x64", "Debug", "domoticz.exe"),
                  os.path.join(REPO_ROOT, "msbuild", "Debug", "domoticz.exe"),
                  os.path.join(REPO_ROOT, "msbuild", "Release", "domoticz.exe"),
                  os.path.join(REPO_ROOT, "domoticz.exe"),
                  os.path.join(REPO_ROOT, "domoticz")]
    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate
    return None


OPENER = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))


def api(base_url, params, form=None):
    url = base_url + "/json.htm?" + urllib.parse.urlencode(params)
    data = urllib.parse.urlencode(form).encode() if form is not None else None
    with OPENER.open(url, data, timeout=15) as answer:
        return json.loads(answer.read().decode("utf-8", "ignore"))


def login(base_url, user, password):
    """A fresh database has no users, so create the admin and start a session."""
    api(base_url, {"type": "command", "param": "setupwizardcreateadmin"},
        {"username": user, "password": password})
    answer = api(base_url, {"type": "command", "param": "logincheck"},
                 {"username": base64.b64encode(user.encode()).decode(), "password": password})
    return answer.get("status") == "OK"


def wait_for_web(base_url, deadline):
    while time.time() < deadline:
        try:
            if api(base_url, {"type": "command", "param": "getversion"}).get("status") == "OK":
                return True
        except Exception:
            time.sleep(1)
    return False


def install_plugin():
    """Copy the example plugin where Domoticz looks for it. Returns True if we created it."""
    if os.path.isdir(PLUGIN_TARGET):
        return False
    shutil.copytree(PLUGIN_SOURCE, PLUGIN_TARGET)
    return True


def run_cases(server, cases):
    """Collect one report per case, in order. Returns (reports, wedged_case)."""
    reports = {}
    timeout = CONNECT_TIMEOUT
    for expected in range(len(cases) + 1):
        try:
            index, report = server.reports.get(timeout=timeout)
        except queue.Empty:
            return reports, cases[expected - 1][0] if expected else "(plugin never connected)"
        timeout = CASE_TIMEOUT
        if index:
            reports[cases[index - 1][0]] = report
    return reports, None


def evaluate(cases, reports, wedged):
    failures = []
    if wedged:
        failures.append("no response after case '%s' - the parser hung or the plugin died" % wedged)

    for name, _, expected in cases:
        if name not in reports:
            if not wedged:
                failures.append("%s: no result reported" % name)
            continue
        fields = parse_result(reports[name])
        if fields.get("case") != name:
            failures.append("%s: not parsed, plugin reported '%s'" % (name, reports[name]))
            continue
        if fields.get("status") != "200":
            failures.append("%s: status is '%s', expected 200" % (name, fields.get("status")))
        if fields.get("body") != "OK":
            failures.append("%s: body is '%s', expected 'OK'" % (name, fields.get("body")))
        for key, value in expected.items():
            if fields.get(key) != value:
                failures.append("%s: header '%s' is '%s', expected '%s'" % (name, key, fields.get(key), value))
    return failures


def main():
    keep_log = "--keep-log" in sys.argv

    executable = find_executable()
    if not executable:
        print("FAIL: no Domoticz executable found, set DOMOTICZ_EXE")
        return 2
    if not os.path.isdir(PLUGIN_SOURCE):
        print("FAIL: missing %s" % PLUGIN_SOURCE)
        return 2

    cases = build_cases()
    server = CaseServer(cases)
    server.start()
    print("Test server listening on 127.0.0.1:%d" % server.port)

    created_plugin = install_plugin()
    work_dir = tempfile.mkdtemp(prefix="dz-hdrtest-")
    log_path = os.path.join(work_dir, "domoticz.log")
    web_port = free_port()
    base_url = "http://127.0.0.1:%d" % web_port
    process = None

    try:
        print("Starting %s on port %d" % (executable, web_port))
        process = subprocess.Popen([executable,
                                    "-www", str(web_port), "-sslwww", "0",
                                    "-dbase", os.path.join(work_dir, "domoticz.db"),
                                    "-log", log_path,
                                    "-nocache", "-nodevcleanup", "-nobrowser", "-nomcp"],
                                   cwd=REPO_ROOT,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        if not wait_for_web(base_url, time.time() + 90):
            print("FAIL: Domoticz did not answer on %s" % base_url)
            return 2

        if not login(base_url, os.environ.get("DOMOTICZ_USER", "admin"),
                     os.environ.get("DOMOTICZ_PASS", "domoticz")):
            print("FAIL: could not create an admin session")
            return 2

        answer = api(base_url, {"type": "command", "param": "addhardware", "htype": HTYPE_PYTHON_PLUGIN,
                                "name": "HttpHeaderTest", "enabled": "true", "loglevel": "1",
                                "address": "127.0.0.1", "port": server.port, "extra": PLUGIN_KEY,
                                "username": "", "password": "", "datatimeout": "0"})
        if answer.get("status") != "OK":
            print("FAIL: could not add the plugin as hardware: %s" % answer)
            return 2

        reports, wedged = run_cases(server, cases)
    finally:
        server.stop()
        if process:
            process.terminate()
            try:
                process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                process.kill()
        if created_plugin:
            shutil.rmtree(PLUGIN_TARGET, ignore_errors=True)

    print("")
    for name, _, _ in cases:
        print("  %-12s %s" % (name, reports.get(name, "<nothing reported>")))
    print("")

    failures = evaluate(cases, reports, wedged)
    if keep_log and os.path.isfile(log_path):
        print("--- Domoticz log ---")
        with open(log_path, "r", encoding="utf-8", errors="ignore") as handle:
            print(handle.read())

    shutil.rmtree(work_dir, ignore_errors=True)

    if failures:
        for failure in failures:
            print("FAIL: %s" % failure)
        return 1

    print("PASS: all %d header parsing cases handled correctly" % len(cases))
    return 0


if __name__ == "__main__":
    sys.exit(main())
