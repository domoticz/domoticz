#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from pyasn1.compat.octets import null
from pysnmp.proto import rfc3411, error
from pysnmp.proto.api import v1, v2c  # backend is always SMIv2 compliant
from pysnmp.proto.proxy import rfc2576
from pysnmp import debug


# 3.4
class NotificationReceiver(object):
    pduTypes = (v1.TrapPDU.tagSet, v2c.SNMPv2TrapPDU.tagSet,
                v2c.InformRequestPDU.tagSet)

    def __init__(self, snmpEngine, cbFun, cbCtx=None):
        snmpEngine.msgAndPduDsp.registerContextEngineId(
            null, self.pduTypes, self.processPdu  # '' is a wildcard
        )
        self.__cbFunVer = 0
        self.__cbFun = cbFun
        self.__cbCtx = cbCtx

    def close(self, snmpEngine):
        snmpEngine.msgAndPduDsp.unregisterContextEngineId(
            null, self.pduTypes
        )
        self.__cbFun = self.__cbCtx = None

    def processPdu(self, snmpEngine, messageProcessingModel,
                   securityModel, securityName, securityLevel,
                   contextEngineId, contextName, pduVersion, PDU,
                   maxSizeResponseScopedPDU, stateReference):

        # Agent-side API complies with SMIv2
        if messageProcessingModel == 0:
            origPdu = PDU
            PDU = rfc2576.v1ToV2(PDU)
        else:
            origPdu = None

        errorStatus = 'noError'
        errorIndex = 0
        varBinds = v2c.apiPDU.getVarBinds(PDU)

        debug.logger & debug.flagApp and debug.logger(
            'processPdu: stateReference %s, varBinds %s' % (stateReference, varBinds))

        # 3.4
        if PDU.tagSet in rfc3411.confirmedClassPDUs:
            # 3.4.1 --> no-op

            rspPDU = v2c.apiPDU.getResponse(PDU)

            # 3.4.2
            v2c.apiPDU.setErrorStatus(rspPDU, errorStatus)
            v2c.apiPDU.setErrorIndex(rspPDU, errorIndex)
            v2c.apiPDU.setVarBinds(rspPDU, varBinds)

            debug.logger & debug.flagApp and debug.logger(
                'processPdu: stateReference %s, confirm PDU %s' % (stateReference, rspPDU.prettyPrint()))

            # Agent-side API complies with SMIv2
            if messageProcessingModel == 0:
                rspPDU = rfc2576.v2ToV1(rspPDU, origPdu)

            statusInformation = {}

            # 3.4.3
            try:
                snmpEngine.msgAndPduDsp.returnResponsePdu(
                    snmpEngine, messageProcessingModel, securityModel,
                    securityName, securityLevel, contextEngineId,
                    contextName, pduVersion, rspPDU, maxSizeResponseScopedPDU,
                    stateReference, statusInformation)

            except error.StatusInformation:
                debug.logger & debug.flagApp and debug.logger(
                    'processPdu: stateReference %s, statusInformation %s' % (stateReference, sys.exc_info()[1]))
                snmpSilentDrops, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols('__SNMPv2-MIB',
                                                                                                         'snmpSilentDrops')
                snmpSilentDrops.syntax += 1

        elif PDU.tagSet in rfc3411.unconfirmedClassPDUs:
            pass
        else:
            raise error.ProtocolError('Unexpected PDU class %s' % PDU.tagSet)

        debug.logger & debug.flagApp and debug.logger(
            'processPdu: stateReference %s, user cbFun %s, cbCtx %s, varBinds %s' % (
                stateReference, self.__cbFun, self.__cbCtx, varBinds))

        if self.__cbFunVer:
            self.__cbFun(snmpEngine, stateReference, contextEngineId,
                         contextName, varBinds, self.__cbCtx)
        else:
            # Compatibility stub (handle legacy cbFun interface)
            try:
                self.__cbFun(snmpEngine, contextEngineId, contextName,
                             varBinds, self.__cbCtx)
            except TypeError:
                self.__cbFunVer = 1
                self.__cbFun(snmpEngine, stateReference, contextEngineId,
                             contextName, varBinds, self.__cbCtx)
