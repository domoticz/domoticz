#!/usr/bin/env python3
"""
mqtt_ad_send.py: replay a capture made with mqtt_ad_record.py (or the older mqtt_ad_record.sh) to an MQTT broker,
for example a test broker with a Domoticz MQTT Auto Discovery gateway on it. Based on mqtt_ad_send.sh by JvdZ.

Runs on Windows, Linux and macOS. Requires Python 3.7+ and paho-mqtt:  pip install paho-mqtt

usage:
  python3 mqtt_ad_send.py [-h host] [-p port] [-u user] [-P password] [--tls] [--cafile file] [--insecure]
                          [-i inputfile] [-s searchstring] [-r y|n] [-w milliseconds] [-d]

examples:
  # Send all records of mqtt_ad_record_all.log to the local broker
  python3 mqtt_ad_send.py

  # Only the records containing "TASMOTA_A1", from a specific file, without sending (dry run)
  python3 mqtt_ad_send.py -i mqtt_ad_record_testDevice.log -s TASMOTA_A1 -d

Do not replay a capture to the broker it came from: the retained discovery configs and states would overwrite
those of the real devices. Use a separate test broker.
"""
import argparse
import ssl
import sys
import time

VERSION = '20261102-01'


def parse_args():
    ap = argparse.ArgumentParser(add_help=False, description='Replay an MQTT AD capture to a broker.')
    ap.add_argument('--help', action='help', help='show this help and exit')
    ap.add_argument('-h', dest='host', default='127.0.0.1', help='hostname or IP address of the MQTT broker (default 127.0.0.1)')
    ap.add_argument('-p', dest='port', type=int, default=None, help='port of the MQTT broker (default 1883, 8883 with --tls)')
    ap.add_argument('-u', dest='user', default='', help='user for the MQTT broker')
    ap.add_argument('-P', dest='password', default='', help='password for the MQTT broker (never shown)')
    ap.add_argument('--tls', action='store_true', help='connect with TLS')
    ap.add_argument('--cafile', default=None, help='CA certificate file for TLS (implies --tls)')
    ap.add_argument('--insecure', action='store_true', help='TLS without checking the certificate (implies --tls)')
    ap.add_argument('-i', dest='infile', default='mqtt_ad_record_all.log', help='input file (default mqtt_ad_record_all.log)')
    ap.add_argument('-s', dest='search', default='', help='only send records that contain this string in the topic or payload (case insensitive)')
    ap.add_argument('-r', dest='retain', choices=('y', 'n'), default=None, help='override the retain flag of the records')
    ap.add_argument('-w', dest='wait', type=int, default=0, help='wait this many milliseconds between messages (default 0)')
    ap.add_argument('-d', dest='dryrun', action='store_true', help='dry run: only show the selected records, send nothing')
    a = ap.parse_args()
    if a.cafile or a.insecure:
        a.tls = True
    if a.port is None:
        a.port = 8883 if a.tls else 1883
    return a


def read_records(a):
    search = a.search.lower()
    out = []
    with open(a.infile, encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.rstrip('\r\n')
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t', 3)
            if len(parts) < 4 or parts[3] == '':
                continue
            _, retain, topic, payload = parts
            if search and search not in topic.lower() and search not in payload.lower():
                continue
            r = (retain.strip() == '1') if a.retain is None else (a.retain == 'y')
            out.append((topic, payload, r))
    return out


def main():
    a = parse_args()
    print('== mqtt_ad_send.py %s ==========================================================' % VERSION)
    print('MQTT broker : %s:%d%s' % (a.host, a.port, ' (TLS)' if a.tls else ''))
    print('Login       : %s%s' % (a.user or '(none)', ' (password set)' if a.password else ''))
    print('Input file  : %s' % a.infile)
    if a.retain:
        print('Retain      : override with %s' % a.retain)
    if a.dryrun:
        print('### Dry run: no MQTT messages are sent, just showing the selected records. ###')
    try:
        records = read_records(a)
    except OSError as e:
        print('!!!! Cannot read %s: %s' % (a.infile, e))
        return 1

    if a.dryrun:
        for n, (topic, payload, r) in enumerate(records, 1):
            print('-- %d:%s\nT=%s\nP=%s' % (n, ' -r' if r else '', topic, payload))
        print('### Dry run ended, %d records selected, nothing was sent. ###' % len(records))
        return 0

    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        print('!!!! The Python package paho-mqtt is not installed. Install it with:  pip install paho-mqtt\n'
              '     (Debian/Raspberry Pi OS: sudo apt-get install python3-paho-mqtt)')
        return 1
    cid = 'mqtt_ad_send-%d' % (int(time.time() * 1000) % 1000000)
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
    state = {'rc': None}
    # paho 1.x passes an int, paho 2.x a ReasonCode (and properties)
    c.on_connect = lambda cl, u, flags, rc, *props: state.update(rc=rc.value if hasattr(rc, 'value') else rc)
    try:
        c.connect(a.host, a.port, keepalive=60)
    except ssl.SSLCertVerificationError as e:
        print('!!!! Cannot connect to %s:%d: %s' % (a.host, a.port, e.verify_message))
        print('     Give the CA certificate with --cafile, or use --insecure for a self signed certificate')
        return 1
    except Exception as e:
        print('!!!! Cannot connect to %s:%d: %s' % (a.host, a.port, e))
        return 1
    c.loop_start()
    for _ in range(100):
        if state['rc'] is not None:
            break
        time.sleep(0.1)
    if state['rc'] != 0:
        print('!!!! Connection refused or no answer (code %s)' % state['rc'])
        c.loop_stop()
        return 1
    try:
        for n, (topic, payload, r) in enumerate(records, 1):
            print('== %d >%s\nT=%s\nP=%s' % (n, ' -r' if r else '', topic, payload))
            c.publish(topic, payload, qos=1, retain=r).wait_for_publish()
            if a.wait:
                time.sleep(a.wait / 1000.0)
    except KeyboardInterrupt:
        print('** Ctrl+C pressed, stopped')
    c.loop_stop()
    c.disconnect()
    print('Sent %d records' % len(records))
    return 0


if __name__ == '__main__':
    sys.exit(main())
