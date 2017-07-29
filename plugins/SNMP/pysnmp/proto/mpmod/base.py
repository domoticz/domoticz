#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto.mpmod import cache
from pysnmp.proto import error


class AbstractMessageProcessingModel(object):
    snmpMsgSpec = NotImplementedError

    def __init__(self):
        self._snmpMsgSpec = self.snmpMsgSpec()  # local copy
        self._cache = cache.Cache()

    def prepareOutgoingMessage(self, snmpEngine, transportDomain,
                               transportAddress, messageProcessingModel,
                               securityModel, securityName, securityLevel,
                               contextEngineId, contextName, pduVersion,
                               pdu, expectResponse, sendPduHandle):
        raise error.ProtocolError('method not implemented')

    def prepareResponseMessage(self, snmpEngine, messageProcessingModel,
                               securityModel, securityName, securityLevel,
                               contextEngineId, contextName, pduVersion,
                               pdu, maxSizeResponseScopedPDU,
                               stateReference, statusInformation):
        raise error.ProtocolError('method not implemented')

    def prepareDataElements(self, snmpEngine, transportDomain,
                            transportAddress, wholeMsg):
        raise error.ProtocolError('method not implemented')

    def releaseStateInformation(self, sendPduHandle):
        try:
            self._cache.popBySendPduHandle(sendPduHandle)
        except error.ProtocolError:
            pass  # XXX maybe these should all follow some scheme?

    def receiveTimerTick(self, snmpEngine, timeNow):
        self._cache.expireCaches()
