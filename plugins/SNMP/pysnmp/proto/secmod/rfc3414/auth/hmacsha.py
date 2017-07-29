#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
try:
    from hashlib import sha1
except ImportError:
    import sha

    sha1 = sha.new
from pyasn1.type import univ
from pysnmp.proto.secmod.rfc3414.auth import base
from pysnmp.proto.secmod.rfc3414 import localkey
from pysnmp.proto import errind, error

_twelveZeros = univ.OctetString((0,) * 12).asOctets()
_fortyFourZeros = (0,) * 44


# 7.2.4

class HmacSha(base.AbstractAuthenticationService):
    serviceID = (1, 3, 6, 1, 6, 3, 10, 1, 1, 3)  # usmHMACSHAAuthProtocol
    __ipad = [0x36] * 64
    __opad = [0x5C] * 64

    def hashPassphrase(self, authKey):
        return localkey.hashPassphraseSHA(authKey)

    def localizeKey(self, authKey, snmpEngineID):
        return localkey.localizeKeySHA(authKey, snmpEngineID)

    # 7.3.1
    def authenticateOutgoingMsg(self, authKey, wholeMsg):
        # 7.3.1.1
        # Here we expect calling secmod to indicate where the digest
        # should be in the substrate. Also, it pre-sets digest placeholder
        # so we hash wholeMsg out of the box.
        # Yes, that's ugly but that's rfc...
        l = wholeMsg.find(_twelveZeros)
        if l == -1:
            raise error.ProtocolError('Cant locate digest placeholder')
        wholeHead = wholeMsg[:l]
        wholeTail = wholeMsg[l + 12:]

        # 7.3.1.2a
        extendedAuthKey = authKey.asNumbers() + _fortyFourZeros

        # 7.3.1.2b -- no-op

        # 7.3.1.2c
        k1 = univ.OctetString(
            map(lambda x, y: x ^ y, extendedAuthKey, self.__ipad)
        )

        # 7.3.1.2d -- no-op

        # 7.3.1.2e
        k2 = univ.OctetString(
            map(lambda x, y: x ^ y, extendedAuthKey, self.__opad)
        )

        # 7.3.1.3
        d1 = sha1(k1.asOctets() + wholeMsg).digest()

        # 7.3.1.4
        d2 = sha1(k2.asOctets() + d1).digest()
        mac = d2[:12]

        # 7.3.1.5 & 6
        return wholeHead + mac + wholeTail

    # 7.3.2
    def authenticateIncomingMsg(self, authKey, authParameters, wholeMsg):
        # 7.3.2.1 & 2
        if len(authParameters) != 12:
            raise error.StatusInformation(
                errorIndication=errind.authenticationError
            )

        # 7.3.2.3
        l = wholeMsg.find(authParameters.asOctets())
        if l == -1:
            raise error.ProtocolError('Cant locate digest in wholeMsg')
        wholeHead = wholeMsg[:l]
        wholeTail = wholeMsg[l + 12:]
        authenticatedWholeMsg = wholeHead + _twelveZeros + wholeTail

        # 7.3.2.4a
        extendedAuthKey = authKey.asNumbers() + _fortyFourZeros

        # 7.3.2.4b --> no-op

        # 7.3.2.4c
        k1 = univ.OctetString(
            map(lambda x, y: x ^ y, extendedAuthKey, self.__ipad)
        )

        # 7.3.2.4d --> no-op

        # 7.3.2.4e
        k2 = univ.OctetString(
            map(lambda x, y: x ^ y, extendedAuthKey, self.__opad)
        )

        # 7.3.2.5a
        d1 = sha1(k1.asOctets() + authenticatedWholeMsg).digest()

        # 7.3.2.5b
        d2 = sha1(k2.asOctets() + d1).digest()

        # 7.3.2.5c
        mac = d2[:12]

        # 7.3.2.6
        if mac != authParameters:
            raise error.StatusInformation(
                errorIndication=errind.authenticationFailure
            )

        return authenticatedWholeMsg
