#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import random
from pysnmp.proto.secmod.rfc3414.priv import base
from pysnmp.proto.secmod.rfc3414.auth import hmacmd5, hmacsha
from pysnmp.proto.secmod.rfc3414 import localkey
from pysnmp.proto import errind, error
from pyasn1.type import univ
from sys import version_info

try:
    from Crypto.Cipher import DES
except ImportError:
    DES = None

random.seed()


# 8.2.4

class Des(base.AbstractEncryptionService):
    serviceID = (1, 3, 6, 1, 6, 3, 10, 1, 2, 2)  # usmDESPrivProtocol
    keySize = 16

    if version_info < (2, 3):
        _localInt = int(random.random() * 0xffffffff)
    else:
        _localInt = random.randrange(0, 0xffffffff)

    def hashPassphrase(self, authProtocol, privKey):
        if authProtocol == hmacmd5.HmacMd5.serviceID:
            return localkey.hashPassphraseMD5(privKey)
        elif authProtocol == hmacsha.HmacSha.serviceID:
            return localkey.hashPassphraseSHA(privKey)
        else:
            raise error.ProtocolError(
                'Unknown auth protocol %s' % (authProtocol,)
            )

    def localizeKey(self, authProtocol, privKey, snmpEngineID):
        if authProtocol == hmacmd5.HmacMd5.serviceID:
            localPrivKey = localkey.localizeKeyMD5(privKey, snmpEngineID)
        elif authProtocol == hmacsha.HmacSha.serviceID:
            localPrivKey = localkey.localizeKeySHA(privKey, snmpEngineID)
        else:
            raise error.ProtocolError(
                'Unknown auth protocol %s' % (authProtocol,)
            )
        return localPrivKey[:self.keySize]

    # 8.1.1.1
    def __getEncryptionKey(self, privKey, snmpEngineBoots):
        desKey = privKey[:8]
        preIV = privKey[8:16]

        securityEngineBoots = int(snmpEngineBoots)

        salt = [securityEngineBoots >> 24 & 0xff,
                securityEngineBoots >> 16 & 0xff,
                securityEngineBoots >> 8 & 0xff,
                securityEngineBoots & 0xff,
                self._localInt >> 24 & 0xff,
                self._localInt >> 16 & 0xff,
                self._localInt >> 8 & 0xff,
                self._localInt & 0xff]
        if self._localInt == 0xffffffff:
            self._localInt = 0
        else:
            self._localInt += 1

        return (desKey.asOctets(),
                univ.OctetString(salt).asOctets(),
                univ.OctetString(map(lambda x, y: x ^ y, salt, preIV.asNumbers())).asOctets())

    @staticmethod
    def __getDecryptionKey(privKey, salt):
        return (privKey[:8].asOctets(),
                univ.OctetString(map(lambda x, y: x ^ y, salt.asNumbers(), privKey[8:16].asNumbers())).asOctets())

    # 8.2.4.1
    def encryptData(self, encryptKey, privParameters, dataToEncrypt):
        if DES is None:
            raise error.StatusInformation(
                errorIndication=errind.encryptionError
            )

        snmpEngineBoots, snmpEngineTime, salt = privParameters

        # 8.3.1.1
        desKey, salt, iv = self.__getEncryptionKey(
            encryptKey, snmpEngineBoots
        )

        # 8.3.1.2
        privParameters = univ.OctetString(salt)

        # 8.1.1.2
        desObj = DES.new(desKey, DES.MODE_CBC, iv)
        plaintext = dataToEncrypt + univ.OctetString((0,) * (8 - len(dataToEncrypt) % 8)).asOctets()
        ciphertext = desObj.encrypt(plaintext)

        # 8.3.1.3 & 4
        return univ.OctetString(ciphertext), privParameters

    # 8.2.4.2
    def decryptData(self, decryptKey, privParameters, encryptedData):
        if DES is None:
            raise error.StatusInformation(
                errorIndication=errind.decryptionError
            )

        snmpEngineBoots, snmpEngineTime, salt = privParameters

        # 8.3.2.1
        if len(salt) != 8:
            raise error.StatusInformation(
                errorIndication=errind.decryptionError
            )

        # 8.3.2.2 no-op

        # 8.3.2.3
        desKey, iv = self.__getDecryptionKey(decryptKey, salt)

        # 8.3.2.4 -> 8.1.1.3
        if len(encryptedData) % 8 != 0:
            raise error.StatusInformation(
                errorIndication=errind.decryptionError
            )

        desObj = DES.new(desKey, DES.MODE_CBC, iv)

        # 8.3.2.6
        return desObj.decrypt(encryptedData.asOctets())
