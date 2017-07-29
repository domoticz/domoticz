#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from pysnmp.proto import rfc1902, rfc1905, rfc3411, errind, error
from pysnmp.proto.api import v2c  # backend is always SMIv2 compliant
from pysnmp.proto.proxy import rfc2576
import pysnmp.smi.error
from pysnmp import debug


# 3.2
class CommandResponderBase(object):
    acmID = 3  # default MIB access control method to use
    pduTypes = ()

    def __init__(self, snmpEngine, snmpContext):
        snmpEngine.msgAndPduDsp.registerContextEngineId(
            snmpContext.contextEngineId, self.pduTypes, self.processPdu
        )
        self.snmpContext = snmpContext
        self.__pendingReqs = {}

    def handleMgmtOperation(self, snmpEngine, stateReference, contextName,
                            PDU, acInfo):
        pass

    def close(self, snmpEngine):
        snmpEngine.msgAndPduDsp.unregisterContextEngineId(
            self.snmpContext.contextEngineId, self.pduTypes
        )
        self.snmpContext = self.__pendingReqs = None

    def sendVarBinds(self, snmpEngine, stateReference,
                     errorStatus, errorIndex, varBinds):
        (messageProcessingModel, securityModel, securityName,
         securityLevel, contextEngineId, contextName,
         pduVersion, PDU, origPdu, maxSizeResponseScopedPDU,
         statusInformation) = self.__pendingReqs[stateReference]

        v2c.apiPDU.setErrorStatus(PDU, errorStatus)
        v2c.apiPDU.setErrorIndex(PDU, errorIndex)
        v2c.apiPDU.setVarBinds(PDU, varBinds)

        debug.logger & debug.flagApp and debug.logger(
            'sendVarBinds: stateReference %s, errorStatus %s, errorIndex %s, varBinds %s' % (
            stateReference, errorStatus, errorIndex, varBinds)
        )

        self.sendPdu(snmpEngine, stateReference, PDU)

    # backward compatibility
    sendRsp = sendVarBinds

    def sendPdu(self, snmpEngine, stateReference, PDU):
        (messageProcessingModel, securityModel, securityName,
         securityLevel, contextEngineId, contextName,
         pduVersion, _, origPdu, maxSizeResponseScopedPDU,
         statusInformation) = self.__pendingReqs[stateReference]

        # Agent-side API complies with SMIv2
        if messageProcessingModel == 0:
            PDU = rfc2576.v2ToV1(PDU, origPdu)

        # 3.2.6
        try:
            snmpEngine.msgAndPduDsp.returnResponsePdu(
                snmpEngine,
                messageProcessingModel,
                securityModel,
                securityName,
                securityLevel,
                contextEngineId,
                contextName,
                pduVersion,
                PDU,
                maxSizeResponseScopedPDU,
                stateReference,
                statusInformation
            )

        except error.StatusInformation:
            debug.logger & debug.flagApp and debug.logger(
                'sendPdu: stateReference %s, statusInformation %s' % (stateReference, sys.exc_info()[1]))
            snmpSilentDrops, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols('__SNMPv2-MIB',
                                                                                                     'snmpSilentDrops')
            snmpSilentDrops.syntax += 1

    _getRequestType = rfc1905.GetRequestPDU.tagSet
    _getNextRequestType = rfc1905.GetNextRequestPDU.tagSet
    _setRequestType = rfc1905.SetRequestPDU.tagSet
    _counter64Type = rfc1902.Counter64.tagSet

    def releaseStateInformation(self, stateReference):
        if stateReference in self.__pendingReqs:
            del self.__pendingReqs[stateReference]

    def processPdu(self, snmpEngine, messageProcessingModel, securityModel,
                   securityName, securityLevel, contextEngineId, contextName,
                   pduVersion, PDU, maxSizeResponseScopedPDU, stateReference):

        # Agent-side API complies with SMIv2
        if messageProcessingModel == 0:
            origPdu = PDU
            PDU = rfc2576.v1ToV2(PDU)
        else:
            origPdu = None

        # 3.2.1
        if (PDU.tagSet not in rfc3411.readClassPDUs and
                PDU.tagSet not in rfc3411.writeClassPDUs):
            raise error.ProtocolError('Unexpected PDU class %s' % PDU.tagSet)

        # 3.2.2 --> no-op

        # 3.2.4
        rspPDU = v2c.apiPDU.getResponse(PDU)

        statusInformation = {}

        self.__pendingReqs[stateReference] = (
            messageProcessingModel, securityModel, securityName,
            securityLevel, contextEngineId, contextName, pduVersion,
            rspPDU, origPdu, maxSizeResponseScopedPDU, statusInformation
        )

        # 3.2.5
        varBinds = v2c.apiPDU.getVarBinds(PDU)
        errorStatus, errorIndex = 'noError', 0

        debug.logger & debug.flagApp and debug.logger(
            'processPdu: stateReference %s, varBinds %s' % (stateReference, varBinds))

        try:
            self.handleMgmtOperation(snmpEngine, stateReference,
                                     contextName, PDU,
                                     (self.__verifyAccess, snmpEngine))
        # SNMPv2 SMI exceptions
        except pysnmp.smi.error.GenError:
            errorIndication = sys.exc_info()[1]
            debug.logger & debug.flagApp and debug.logger(
                'processPdu: stateReference %s, errorIndication %s' % (stateReference, errorIndication))
            if 'oid' in errorIndication:
                # Request REPORT generation
                statusInformation['oid'] = errorIndication['oid']
                statusInformation['val'] = errorIndication['val']

        # PDU-level SMI errors
        except pysnmp.smi.error.NoAccessError:
            errorStatus, errorIndex = 'noAccess', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.WrongTypeError:
            errorStatus, errorIndex = 'wrongType', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.WrongLengthError:
            errorStatus, errorIndex = 'wrongLength', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.WrongEncodingError:
            errorStatus, errorIndex = 'wrongEncoding', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.WrongValueError:
            errorStatus, errorIndex = 'wrongValue', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.NoCreationError:
            errorStatus, errorIndex = 'noCreation', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.InconsistentValueError:
            errorStatus, errorIndex = 'inconsistentValue', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.ResourceUnavailableError:
            errorStatus, errorIndex = 'resourceUnavailable', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.CommitFailedError:
            errorStatus, errorIndex = 'commitFailed', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.UndoFailedError:
            errorStatus, errorIndex = 'undoFailed', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.AuthorizationError:
            errorStatus, errorIndex = 'authorizationError', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.NotWritableError:
            errorStatus, errorIndex = 'notWritable', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.InconsistentNameError:
            errorStatus, errorIndex = 'inconsistentName', sys.exc_info()[1]['idx'] + 1
        except pysnmp.smi.error.SmiError:
            errorStatus, errorIndex = 'genErr', len(varBinds) and 1 or 0
        except pysnmp.error.PySnmpError:
            self.releaseStateInformation(stateReference)
            return
        else:  # successful request processor must release state info
            return

        self.sendVarBinds(snmpEngine, stateReference, errorStatus,
                          errorIndex, varBinds)

        self.releaseStateInformation(stateReference)

    def __verifyAccess(self, name, syntax, idx, viewType, acCtx):
        snmpEngine = acCtx
        execCtx = snmpEngine.observer.getExecutionContext('rfc3412.receiveMessage:request')
        (securityModel, securityName, securityLevel, contextName,
         pduType) = (execCtx['securityModel'], execCtx['securityName'],
                     execCtx['securityLevel'], execCtx['contextName'],
                     execCtx['pdu'].getTagSet())
        try:
            snmpEngine.accessControlModel[self.acmID].isAccessAllowed(
                snmpEngine, securityModel, securityName,
                securityLevel, viewType, contextName, name
            )
        # Map ACM errors onto SMI ones
        except error.StatusInformation:
            statusInformation = sys.exc_info()[1]
            debug.logger & debug.flagApp and debug.logger(
                '__verifyAccess: name %s, statusInformation %s' % (name, statusInformation))
            errorIndication = statusInformation['errorIndication']
            # 3.2.5...
            if (errorIndication == errind.noSuchView or
                    errorIndication == errind.noAccessEntry or
                    errorIndication == errind.noGroupName):
                raise pysnmp.smi.error.AuthorizationError(name=name, idx=idx)
            elif errorIndication == errind.otherError:
                raise pysnmp.smi.error.GenError(name=name, idx=idx)
            elif errorIndication == errind.noSuchContext:
                snmpUnknownContexts, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                    '__SNMP-TARGET-MIB', 'snmpUnknownContexts')
                snmpUnknownContexts.syntax += 1
                # Request REPORT generation
                raise pysnmp.smi.error.GenError(name=name, idx=idx,
                                                oid=snmpUnknownContexts.name,
                                                val=snmpUnknownContexts.syntax)
            elif errorIndication == errind.notInView:
                return 1
            else:
                raise error.ProtocolError('Unknown ACM error %s' % errorIndication)
        else:
            # rfc2576: 4.1.2.1
            if (securityModel == 1 and syntax is not None and
                    self._counter64Type == syntax.getTagSet() and
                    self._getNextRequestType == pduType):
                # This will cause MibTree to skip this OID-value
                raise pysnmp.smi.error.NoAccessError(name=name, idx=idx)


class GetCommandResponder(CommandResponderBase):
    pduTypes = (rfc1905.GetRequestPDU.tagSet,)

    # rfc1905: 4.2.1
    def handleMgmtOperation(self, snmpEngine, stateReference,
                            contextName, PDU, acInfo):
        (acFun, acCtx) = acInfo
        # rfc1905: 4.2.1.1
        mgmtFun = self.snmpContext.getMibInstrum(contextName).readVars
        self.sendVarBinds(snmpEngine, stateReference, 0, 0,
                          mgmtFun(v2c.apiPDU.getVarBinds(PDU), (acFun, acCtx)))
        self.releaseStateInformation(stateReference)


class NextCommandResponder(CommandResponderBase):
    pduTypes = (rfc1905.GetNextRequestPDU.tagSet,)

    # rfc1905: 4.2.2
    def handleMgmtOperation(self, snmpEngine, stateReference,
                            contextName, PDU, acInfo):
        (acFun, acCtx) = acInfo
        # rfc1905: 4.2.2.1
        mgmtFun = self.snmpContext.getMibInstrum(contextName).readNextVars
        varBinds = v2c.apiPDU.getVarBinds(PDU)
        while True:
            rspVarBinds = mgmtFun(varBinds, (acFun, acCtx))
            try:
                self.sendVarBinds(snmpEngine, stateReference, 0, 0, rspVarBinds)
            except error.StatusInformation:
                idx = sys.exc_info()[1]['idx']
                varBinds[idx] = (rspVarBinds[idx][0], varBinds[idx][1])
            else:
                break
        self.releaseStateInformation(stateReference)


class BulkCommandResponder(CommandResponderBase):
    pduTypes = (rfc1905.GetBulkRequestPDU.tagSet,)
    maxVarBinds = 64

    # rfc1905: 4.2.3
    def handleMgmtOperation(self, snmpEngine, stateReference,
                            contextName, PDU, acInfo):
        (acFun, acCtx) = acInfo
        nonRepeaters = v2c.apiBulkPDU.getNonRepeaters(PDU)
        if nonRepeaters < 0:
            nonRepeaters = 0
        maxRepetitions = v2c.apiBulkPDU.getMaxRepetitions(PDU)
        if maxRepetitions < 0:
            maxRepetitions = 0

        reqVarBinds = v2c.apiPDU.getVarBinds(PDU)

        N = min(int(nonRepeaters), len(reqVarBinds))
        M = int(maxRepetitions)
        R = max(len(reqVarBinds) - N, 0)

        if R:
            M = min(M, self.maxVarBinds / R)

        debug.logger & debug.flagApp and debug.logger('handleMgmtOperation: N %d, M %d, R %d' % (N, M, R))

        mgmtFun = self.snmpContext.getMibInstrum(contextName).readNextVars

        if N:
            rspVarBinds = mgmtFun(reqVarBinds[:N], (acFun, acCtx))
        else:
            rspVarBinds = []

        varBinds = reqVarBinds[-R:]
        while M and R:
            rspVarBinds.extend(mgmtFun(varBinds, (acFun, acCtx)))
            varBinds = rspVarBinds[-R:]
            M -= 1

        if len(rspVarBinds):
            self.sendVarBinds(snmpEngine, stateReference, 0, 0, rspVarBinds)
            self.releaseStateInformation(stateReference)
        else:
            raise pysnmp.smi.error.SmiError()


class SetCommandResponder(CommandResponderBase):
    pduTypes = (rfc1905.SetRequestPDU.tagSet,)

    # rfc1905: 4.2.5
    def handleMgmtOperation(self, snmpEngine, stateReference,
                            contextName, PDU, acInfo):
        (acFun, acCtx) = acInfo
        mgmtFun = self.snmpContext.getMibInstrum(contextName).writeVars
        # rfc1905: 4.2.5.1-13
        try:
            self.sendVarBinds(snmpEngine, stateReference, 0, 0,
                              mgmtFun(v2c.apiPDU.getVarBinds(PDU),
                                      (acFun, acCtx)))
            self.releaseStateInformation(stateReference)
        except (pysnmp.smi.error.NoSuchObjectError,
                pysnmp.smi.error.NoSuchInstanceError):
            e = pysnmp.smi.error.NotWritableError()
            e.update(sys.exc_info()[1])
            raise e
