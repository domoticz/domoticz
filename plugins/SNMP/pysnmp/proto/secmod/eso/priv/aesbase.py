#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto.secmod.rfc3826.priv import aes
from pysnmp.proto.secmod.rfc3414.auth import hmacmd5, hmacsha
from pysnmp.proto.secmod.rfc3414 import localkey
from pysnmp.proto import error
from math import ceil

try:
    from hashlib import md5, sha1
except ImportError:
    import md5
    import sha

    md5 = md5.new
    sha1 = sha.new


class AbstractAesBlumenthal(aes.Aes):
    serviceID = ()
    keySize = 0

    # 3.1.2.1
    def localizeKey(self, authProtocol, privKey, snmpEngineID):
        if authProtocol == hmacmd5.HmacMd5.serviceID:
            localPrivKey = localkey.localizeKeyMD5(privKey, snmpEngineID)
            for count in range(1, int(ceil(self.keySize * 1.0 / len(localPrivKey)))):
                # noinspection PyDeprecation,PyCallingNonCallable
                localPrivKey += md5(localPrivKey).digest()
        elif authProtocol == hmacsha.HmacSha.serviceID:
            localPrivKey = localkey.localizeKeySHA(privKey, snmpEngineID)
            # RFC mentions this algo generates 480bit key, but only up to 256 bits are used
            for count in range(1, int(ceil(self.keySize * 1.0 / len(localPrivKey)))):
                localPrivKey += sha1(localPrivKey).digest()
        else:
            raise error.ProtocolError(
                'Unknown auth protocol %s' % (authProtocol,)
            )
        return localPrivKey[:self.keySize]


class AbstractAesReeder(aes.Aes):
    """AES encryption with non-standard key localization.

    Many vendors (including Cisco) do not use:

    https://tools.itef.org/pdf/draft_bluementhal-aes-usm-04.txt

    for key localization instead, they use the procedure for 3DES key localization
    specified in:

    https://tools.itef.org/pdf/draft_reeder_snmpv3-usm-3desede-00.pdf

    The difference between the two is that the Reeder draft does key extension by repeating
    the steps in the password to key algorithm (hash phrase, then localize with SNMPEngine ID).
    """
    serviceID = ()
    keySize = 0

    # 2.1 of https://tools.itef.org/pdf/draft_bluementhal-aes-usm-04.txt
    def localizeKey(self, authProtocol, privKey, snmpEngineID):
        if authProtocol == hmacmd5.HmacMd5.serviceID:
            localPrivKey = localkey.localizeKeyMD5(privKey, snmpEngineID)
            # now extend this key if too short by repeating steps that includes the hashPassphrase step
            while len(localPrivKey) < self.keySize:
                newKey = localkey.hashPassphraseMD5(localPrivKey)  # this is the difference between reeder and bluementhal
                localPrivKey += localkey.localizeKeyMD5(newKey, snmpEngineID)
        elif authProtocol == hmacsha.HmacSha.serviceID:
            localPrivKey = localkey.localizeKeySHA(privKey, snmpEngineID)
            while len(localPrivKey) < self.keySize:
                newKey = localkey.hashPassphraseSHA(localPrivKey)
                localPrivKey += localkey.localizeKeySHA(newKey, snmpEngineID)
        else:
            raise error.ProtocolError(
                'Unknown auth protocol %s' % (authProtocol,)
            )
        return localPrivKey[:self.keySize]
