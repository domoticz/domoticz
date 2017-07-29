#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
try:
    from hashlib import md5, sha1
except ImportError:
    import md5
    import sha

    md5 = md5.new
    sha1 = sha.new
from pyasn1.type import univ


# RFC3414: A.2.1
def hashPassphraseMD5(passphrase):
    passphrase = univ.OctetString(passphrase).asOctets()
    # noinspection PyDeprecation,PyCallingNonCallable
    md = md5()
    ringBuffer = passphrase * (passphrase and (64 // len(passphrase) + 1) or 1)
    # noinspection PyTypeChecker
    ringBufferLen = len(ringBuffer)
    count = 0
    mark = 0
    while count < 16384:
        e = mark + 64
        if e < ringBufferLen:
            md.update(ringBuffer[mark:e])
            mark = e
        else:
            md.update(
                ringBuffer[mark:ringBufferLen] + ringBuffer[0:e - ringBufferLen]
            )
            mark = e - ringBufferLen
        count += 1
    return md.digest()


def localizeKeyMD5(passKey, snmpEngineId):
    passKey = univ.OctetString(passKey).asOctets()
    # noinspection PyDeprecation,PyCallingNonCallable
    return md5(passKey + snmpEngineId.asOctets() + passKey).digest()


def passwordToKeyMD5(passphrase, snmpEngineId):
    return localizeKeyMD5(hashPassphraseMD5(passphrase), snmpEngineId)


# RFC3414: A.2.2
def hashPassphraseSHA(passphrase):
    passphrase = univ.OctetString(passphrase).asOctets()
    md = sha1()
    ringBuffer = passphrase * (64 // len(passphrase) + 1)
    # noinspection PyTypeChecker
    ringBufferLen = len(ringBuffer)
    count = 0
    mark = 0
    while count < 16384:
        e = mark + 64
        if e < ringBufferLen:
            md.update(ringBuffer[mark:e])
            mark = e
        else:
            md.update(
                ringBuffer[mark:ringBufferLen] + ringBuffer[0:e - ringBufferLen]
            )
            mark = e - ringBufferLen
        count += 1
    return md.digest()


def localizeKeySHA(passKey, snmpEngineId):
    passKey = univ.OctetString(passKey).asOctets()
    return sha1(passKey + snmpEngineId.asOctets() + passKey).digest()


def passwordToKeySHA(passphrase, snmpEngineId):
    return localizeKeySHA(hashPassphraseSHA(passphrase), snmpEngineId)
