#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto.secmod.rfc3414.priv import base
from pysnmp.proto import errind, error


class NoPriv(base.AbstractEncryptionService):
    serviceID = (1, 3, 6, 1, 6, 3, 10, 1, 2, 1)  # usmNoPrivProtocol

    def hashPassphrase(self, authProtocol, privKey):
        return

    def localizeKey(self, authProtocol, privKey, snmpEngineID):
        return

    def encryptData(self, encryptKey, privParameters, dataToEncrypt):
        raise error.StatusInformation(errorIndication=errind.noEncryption)

    def decryptData(self, decryptKey, privParameters, encryptedData):
        raise error.StatusInformation(errorIndication=errind.noEncryption)
