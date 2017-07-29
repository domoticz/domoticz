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
from pyasn1.compat.octets import null
from math import ceil

try:
    from hashlib import md5, sha1
except ImportError:
    import md5
    import sha

    md5 = md5.new
    sha1 = sha.new

try:
    from Crypto.Cipher import DES3
except ImportError:
    DES3 = None

random.seed()


# 5.1.1

class Des3(base.AbstractEncryptionService):
    """Reeder 3DES-EDE for USM (Internet draft).

       https://tools.ietf.org/html/draft-reeder-snmpv3-usm-3desede-00
    """
    serviceID = (1, 3, 6, 1, 6, 3, 10, 1, 2, 3)  # usm3DESEDEPrivProtocol
    keySize = 32
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

    # 2.1
    def localizeKey(self, authProtocol, privKey, snmpEngineID):
        if authProtocol == hmacmd5.HmacMd5.serviceID:
            localPrivKey = localkey.localizeKeyMD5(privKey, snmpEngineID)
            # now extend this key if too short by repeating steps that includes the hashPassphrase step
            while len(localPrivKey) < self.keySize:
                newKey = localkey.hashPassphraseMD5(localPrivKey)
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

    # 5.1.1.1
    def __getEncryptionKey(self, privKey, snmpEngineBoots):
        # 5.1.1.1.1
        des3Key = privKey[:24]
        preIV = privKey[24:32]

        securityEngineBoots = int(snmpEngineBoots)

        salt = [
            securityEngineBoots >> 24 & 0xff,
            securityEngineBoots >> 16 & 0xff,
            securityEngineBoots >> 8 & 0xff,
            securityEngineBoots & 0xff,
            self._localInt >> 24 & 0xff,
            self._localInt >> 16 & 0xff,
            self._localInt >> 8 & 0xff,
            self._localInt & 0xff
        ]
        if self._localInt == 0xffffffff:
            self._localInt = 0
        else:
            self._localInt += 1

        # salt not yet hashed XXX

        return (des3Key.asOctets(),
                univ.OctetString(salt).asOctets(),
                univ.OctetString(map(lambda x, y: x ^ y, salt, preIV.asNumbers())).asOctets())

    @staticmethod
    def __getDecryptionKey(privKey, salt):
        return (privKey[:24].asOctets(),
                univ.OctetString(map(lambda x, y: x ^ y, salt.asNumbers(), privKey[24:32].asNumbers())).asOctets())

    # 5.1.1.2
    def encryptData(self, encryptKey, privParameters, dataToEncrypt):
        if DES3 is None:
            raise error.StatusInformation(
                errorIndication=errind.encryptionError
            )

        snmpEngineBoots, snmpEngineTime, salt = privParameters

        des3Key, salt, iv = self.__getEncryptionKey(
            encryptKey, snmpEngineBoots
        )

        des3Obj = DES3.new(des3Key, DES3.MODE_CBC, iv)

        privParameters = univ.OctetString(salt)

        plaintext = dataToEncrypt + univ.OctetString((0,) * (8 - len(dataToEncrypt) % 8)).asOctets()
        ciphertext = des3Obj.encrypt(plaintext)

        return univ.OctetString(ciphertext), privParameters

    # 5.1.1.3
    def decryptData(self, decryptKey, privParameters, encryptedData):
        if DES3 is None:
            raise error.StatusInformation(
                errorIndication=errind.decryptionError
            )
        snmpEngineBoots, snmpEngineTime, salt = privParameters

        if len(salt) != 8:
            raise error.StatusInformation(
                errorIndication=errind.decryptionError
            )

        des3Key, iv = self.__getDecryptionKey(decryptKey, salt)

        if len(encryptedData) % 8 != 0:
            raise error.StatusInformation(
                errorIndication=errind.decryptionError
            )

        des3Obj = DES3.new(des3Key, DES3.MODE_CBC, iv)

        ciphertext = encryptedData.asOctets()
        plaintext = des3Obj.decrypt(ciphertext)

        return plaintext
