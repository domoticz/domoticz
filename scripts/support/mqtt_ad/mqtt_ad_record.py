#!/usr/bin/env python3
"""
mqtt_ad_record.py: capture Home Assistant MQTT Auto Discovery messages into a file that can be shared for
debugging and replayed with mqtt_ad_send.py. Based on mqtt_ad_record.sh by JvdZ.

By default it keeps every discovery config (<prefix>/<component>/[<node_id>/]<object_id>/config) and every
message on a topic that one of those configs points to (state_topic, brightness_state_topic, rgb_state_topic,
availability, json_attributes_topic, ...), whatever the topic is called. Messages of other applications on the
same broker are left out. The "~" base topic and abbreviated keys (stat_t, bri_stat_t, ...) are understood.

Runs on Windows, Linux and macOS. Requires Python 3.7+ and paho-mqtt:  pip install paho-mqtt

usage:
  python3 mqtt_ad_record.py [-h host] [-p port] [-u user] [-P password] [--tls] [--cafile file] [--insecure]
                            [-d prefix] [-s searchstring] [-t seconds] [-a] [-l] [-x] [-X "name1,name2"] [-o file]

examples:
  # Record all discovery configs and their state messages for 10 minutes
  python3 mqtt_ad_record.py

  # Broker with a login and TLS, 5 minutes, latest message per topic only, anonymized
  python3 mqtt_ad_record.py -h 192.168.1.10 -p 8883 -u UserId -P Password --tls -t 300 -l -x

  # Also hide room names
  python3 mqtt_ad_record.py -x -X "Living Room,Kitchen,Garage"

  # Only the devices with "TASMOTA_A1" in their config, for 30 seconds
  python3 mqtt_ad_record.py -s TASMOTA_A1 -t 30

Output file: one message per line, <time><TAB><retain><TAB><topic><TAB><payload>, discovery configs first.
Lines starting with # are comments with a summary. Multi line JSON payloads are written on one line.
"""
import argparse
import json
import re
import ssl
import sys
import time
from collections import OrderedDict
from datetime import datetime

VERSION = '20261102-01'

try:
    import paho.mqtt.client as mqtt
except ImportError:
    sys.exit('!!!! The Python package paho-mqtt is not installed. Install it with:  pip install paho-mqtt\n'
             '     (Debian/Raspberry Pi OS: sudo apt-get install python3-paho-mqtt)')


def parse_args():
    ap = argparse.ArgumentParser(add_help=False, description='Capture MQTT Auto Discovery messages to a file.',
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--help', action='help', help='show this help and exit')
    ap.add_argument('-h', dest='host', default='127.0.0.1', help='hostname or IP address of the MQTT broker (default 127.0.0.1)')
    ap.add_argument('-p', dest='port', type=int, default=None, help='port of the MQTT broker (default 1883, 8883 with --tls)')
    ap.add_argument('-u', dest='user', default='', help='user for the MQTT broker')
    ap.add_argument('-P', dest='password', default='', help='password for the MQTT broker (never shown or written to the file)')
    ap.add_argument('--tls', action='store_true', help='connect with TLS')
    ap.add_argument('--cafile', default=None, help='CA certificate file for TLS (implies --tls); default: the system certificates')
    ap.add_argument('--insecure', action='store_true', help='TLS without checking the certificate (self signed, implies --tls)')
    ap.add_argument('-d', dest='prefix', default='homeassistant', help='discovery prefix (default homeassistant)')
    ap.add_argument('-s', dest='search', default='', help='only keep devices whose discovery config contains this string '
                    '(case insensitive), with the topics their configs point to; with -a: messages containing it')
    ap.add_argument('-t', dest='seconds', type=int, default=600, help='capture time in seconds (default 600); Ctrl+C stops earlier and still writes the file')
    ap.add_argument('-a', dest='keepall', action='store_true', help='keep all messages on the broker, not only the discovery related ones')
    ap.add_argument('-l', dest='lastonly', action='store_true', help='keep only the last message of each topic')
    ap.add_argument('-x', dest='anon', action='store_true', help='anonymize IP addresses, MAC addresses, Z-Wave home ids and Zigbee IEEE addresses '
                    '(the same value always gets the same placeholder, so the topics in the configs still match)')
    ap.add_argument('-X', dest='words', default='', help='anonymize these names as well, comma separated (room or person names; implies -x); '
                    'spaces also match "_" and "-" variants, upper and lower case')
    ap.add_argument('-o', dest='outfile', default='', help='output file (default mqtt_ad_record_all.log, or mqtt_ad_record_<search>.log with -s)')
    a = ap.parse_args()
    a.prefix = a.prefix.rstrip('/')
    if a.cafile or a.insecure:
        a.tls = True
    if a.port is None:
        a.port = 8883 if a.tls else 1883
    if a.words:
        a.anon = True
    if not a.outfile:
        a.outfile = 'mqtt_ad_record_%s.log' % (re.sub(r'[^A-Za-z0-9_-]', '', a.search) if a.search else 'all')
    return a


def make_client(a, name):
    cid = '%s-%d' % (name, int(time.time() * 1000) % 1000000)
    if hasattr(mqtt, 'CallbackAPIVersion'):  # paho-mqtt 2.x
        c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=cid)
    else:
        c = mqtt.Client(client_id=cid)
    if a.user:
        c.username_pw_set(a.user, a.password or None)
    if a.tls:
        c.tls_set(ca_certs=a.cafile, cert_reqs=ssl.CERT_NONE if a.insecure else ssl.CERT_REQUIRED)
        if a.insecure:
            c.tls_insecure_set(True)
    return c


CONNACK = {1: 'unacceptable protocol version', 2: 'client id rejected', 3: 'broker unavailable',
           4: 'bad user name or password', 5: 'not authorized'}


def capture(a):
    msgs = []
    state = {'rc': None}

    def on_connect(c, u, flags, rc, *properties):
        # paho 1.x passes an int, paho 2.x a ReasonCode
        rc = rc.value if hasattr(rc, 'value') else rc
        state['rc'] = rc
        if rc == 0:
            c.subscribe('#', qos=0)

    def on_message(c, u, m):
        try:
            p = m.payload.decode('utf-8')
        except UnicodeDecodeError:
            p = m.payload.decode('latin-1')
        msgs.append((datetime.now().isoformat(timespec='seconds'), 1 if m.retain else 0, m.topic, p))

    c = make_client(a, 'mqtt_ad_record')
    c.on_connect = on_connect
    c.on_message = on_message
    try:
        c.connect(a.host, a.port, keepalive=60)
    except ssl.SSLCertVerificationError as e:
        print('!!!! Cannot connect to %s:%d: %s' % (a.host, a.port, e.verify_message))
        print('     Give the CA certificate with --cafile, or use --insecure for a self signed certificate')
        return None
    except Exception as e:
        print('!!!! Cannot connect to %s:%d: %s' % (a.host, a.port, e))
        return None
    c.loop_start()
    end = time.time() + a.seconds
    shown = -1
    try:
        while time.time() < end:
            time.sleep(0.5)
            if state['rc'] not in (None, 0):
                print('!!!! Connection refused: %s' % CONNACK.get(state['rc'], 'code %s' % state['rc']))
                break
            if len(msgs) != shown:
                shown = len(msgs)
                sys.stdout.write('\r%d messages received, %d seconds left ' % (shown, max(0, end - time.time())))
                sys.stdout.flush()
    except KeyboardInterrupt:
        print('\n** Ctrl+C pressed, writing the file')
    c.loop_stop()
    c.disconnect()
    print('\r%d messages received%s' % (len(msgs), ' ' * 30))
    if state['rc'] is None:
        print('!!!! No answer from the broker (TLS port without --tls, or the other way around?)')
    return msgs


COMMAND = ('command', 'cmd_t', 'cmd_tpl', 'set_position', 'set_pos')


def topics_of(obj, base, out):
    # every *_topic / *_t value (and availability "topic"), with "~" expanded; command topics are not needed
    if isinstance(obj, dict):
        b = obj.get('~', base)
        for k, v in obj.items():
            if isinstance(v, str) and (k == 'topic' or k.endswith('_topic') or k.endswith('_t')) and not any(x in k for x in COMMAND):
                if b and v.startswith('~'):
                    v = b + v[1:]
                elif b and v.endswith('~'):
                    v = v[:-1] + b
                out.add(v)
            elif isinstance(v, (dict, list)):
                topics_of(v, b, out)
    elif isinstance(obj, list):
        for v in obj:
            topics_of(v, base, out)


def match_topic(filt, topic):
    if '+' not in filt and '#' not in filt:
        return filt == topic
    f, t = filt.split('/'), topic.split('/')
    for i, part in enumerate(f):
        if part == '#':
            return True
        if i >= len(t) or (part != '+' and part != t[i]):
            return False
    return len(f) == len(t)


class Anonymizer:
    PATTERNS = [
        ('ip', re.compile(r'(?<![\d.])(?:(?:25[0-5]|2[0-4]\d|1?\d?\d)\.){3}(?:25[0-5]|2[0-4]\d|1?\d?\d)(?![\d.])'),
         lambda n: '192.0.2.%d' % n),
        ('mac', re.compile(r'(?i)(?<![0-9a-f:-])(?:[0-9a-f]{2}([:-]))(?:[0-9a-f]{2}\1){4}[0-9a-f]{2}(?![0-9a-f:-])'),
         lambda n: '02:00:00:00:%02x:%02x' % (n // 256, n % 256)),
        # also inside ids like zwavejs2mqtt_0xc15d8aa6_37-38-0-currentValue, so no \b
        ('ieee', re.compile(r'(?i)(?<![0-9a-z])0x[0-9a-f]{16}(?![0-9a-f])'), lambda n: '0x00000000%08x' % n),
        ('homeid', re.compile(r'(?i)(?<![0-9a-z])0x[0-9a-f]{8}(?![0-9a-f])'), lambda n: '0x%08x' % (0xa0000000 + n)),
    ]

    def __init__(self, words):
        self.mapping, self.counters, self.words = {}, {}, []
        for i, w in enumerate([w.strip() for w in words.split(',') if w.strip()], 1):
            ph = 'Name%d' % i
            variants = {w, w.replace(' ', '_'), w.replace(' ', '-'), w.replace(' ', '')}
            variants |= {v.lower() for v in variants} | {v.upper() for v in variants}
            for v in sorted(variants, key=len, reverse=True):
                self.words.append((v, ph.lower() if v.islower() else ph))

    def _placeholder(self, kind, value, fmt):
        key = (kind, value)
        if key not in self.mapping:
            self.counters[kind] = self.counters.get(kind, 0) + 1
            self.mapping[key] = fmt(self.counters[kind])
        return self.mapping[key]

    def __call__(self, s):
        for kind, rx, fmt in self.PATTERNS:
            s = rx.sub(lambda mo: self._placeholder(kind, mo.group(0).lower(), fmt), s)
        for w, ph in self.words:
            s = s.replace(w, ph)
        return s


def oneline(p):
    if '\n' in p or '\r' in p or '\t' in p:
        try:
            return json.dumps(json.loads(p), ensure_ascii=False, separators=(',', ':'))
        except ValueError:
            return p.replace('\r', ' ').replace('\n', ' ').replace('\t', ' ')
    return p


def main():
    a = parse_args()
    print('== mqtt_ad_record.py %s ==========================================================' % VERSION)
    print('MQTT broker : %s:%d%s' % (a.host, a.port, ' (TLS%s)' % (', no certificate check' if a.insecure else '') if a.tls else ''))
    print('Login       : %s%s' % (a.user or '(none)', ' (password set)' if a.password else ''))
    print('Prefix      : %s' % a.prefix)
    print('Record time : %d seconds (Ctrl+C stops earlier)' % a.seconds)
    print('Search for  : %s' % (a.search or '(all devices)'))
    print('Keep        : %s%s' % ('all messages' if a.keepall else 'discovery configs and the topics they use',
                                  ', last message per topic' if a.lastonly else ''))
    print('Anonymize   : %s' % (('yes' + (', plus names: ' + a.words if a.words else '')) if a.anon else 'no'))
    print('Output file : %s' % a.outfile)

    msgs = capture(a)
    if msgs is None:
        return 1
    search = a.search.lower()
    cfg_re = re.compile(r'^' + re.escape(a.prefix) + r'/[^/]+/(?:[^/]+/)?[^/]+/config$')

    def is_config(topic):
        return bool(cfg_re.match(topic))

    configs, referenced, components = {}, set(), {}
    for _, _, topic, p in msgs:
        if is_config(topic):
            configs[topic] = p
    for topic, p in configs.items():
        if search and search not in topic.lower() and search not in p.lower():
            continue
        try:
            cfg = json.loads(p) if p else {}
        except ValueError:
            cfg = {}
        topics_of(cfg, None, referenced)
        comp = topic[len(a.prefix) + 1:].split('/')[0]
        components[comp] = components.get(comp, 0) + 1
    wild = [t for t in referenced if '+' in t or '#' in t]

    def keep(topic, p):
        if a.keepall or is_config(topic):
            return not search or search in topic.lower() or search in p.lower()
        return topic in referenced or any(match_topic(w, topic) for w in wild)

    sel = [m for m in msgs if keep(m[2], m[3])]
    if a.lastonly:
        last = OrderedDict()
        for m in sel:
            last.pop(m[2], None)
            last[m[2]] = m
        sel = list(last.values())
    # configs first, so a replay creates the devices before their states arrive
    sel.sort(key=lambda m: 0 if is_config(m[2]) else 1)
    anon = Anonymizer(a.words) if a.anon else None
    if anon:
        sel = [(t, r, anon(topic), anon(p)) for t, r, topic, p in sel]

    seen = {m[2] for m in msgs}
    missing = sorted(t for t in referenced if '+' not in t and '#' not in t and t not in seen)
    ncfg = sum(1 for m in sel if is_config(m[2]))
    nother = len(sel) - ncfg
    ntopics = len({m[2] for m in sel if not is_config(m[2])})
    comps = ', '.join('%s %d' % kv for kv in sorted(components.items(), key=lambda kv: -kv[1]))
    with open(a.outfile, 'w', encoding='utf-8', newline='\n') as f:
        f.write('# mqtt_ad_record.py %s, prefix %s%s%s%s%s\n' % (VERSION, a.prefix, ', search "%s"' % a.search if a.search else '',
                ', all messages' if a.keepall else '', ', last per topic' if a.lastonly else '', ', anonymized' if anon else ''))
        f.write('# %d discovery configs (%s), %d other messages on %d topics\n' % (ncfg, comps, nother, ntopics))
        if not a.keepall:
            f.write('# %d topics referenced by the configs, %d of them had no message during the capture\n' % (len(referenced), len(missing)))
        for t, r, topic, p in sel:
            f.write('%s\t%d\t%s\t%s\n' % (t, r, topic, oneline(p)))

    print('Discovery configs : %d' % ncfg)
    if comps:
        print('  by component    : %s' % comps)
    print('Other messages    : %d on %d topics' % (nother, ntopics))
    if not a.keepall:
        print('Referenced topics : %d, without a message during the capture: %d' % (len(referenced), len(missing)))
        if missing:
            print('  (states that are not retained only arrive when the device reports; record longer or operate the devices)')
    if anon:
        print('Anonymized values : %d' % len(anon.mapping))
    print('Capture ended, check file: %s' % a.outfile)
    return 0


if __name__ == '__main__':
    sys.exit(main())
