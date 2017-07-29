#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto.secmod import cache
from pysnmp.proto import error


class AbstractSecurityModel(object):
    securityModelID = None

    def __init__(self):
        self._cache = cache.Cache()

    def processIncomingMsg(self, snmpEngine, messageProcessingModel,
                           maxMessageSize, securityParameters,
                           securityModel, securityLevel, wholeMsg, msg):
        raise error.ProtocolError('Security model %s not implemented' % self)

    def generateRequestMsg(self, snmpEngine, messageProcessingModel,
                           globalData, maxMessageSize, securityModel,
                           securityEngineID, securityName, securityLevel,
                           scopedPDU):
        raise error.ProtocolError('Security model %s not implemented' % self)

    def generateResponseMsg(self, snmpEngine, messageProcessingModel,
                            globalData, maxMessageSize, securityModel,
                            securityEngineID, securityName, securityLevel,
                            scopedPDU, securityStateReference):
        raise error.ProtocolError('Security model %s not implemented' % self)

    def releaseStateInformation(self, stateReference):
        self._cache.pop(stateReference)

    def receiveTimerTick(self, snmpEngine, timeNow):
        pass
