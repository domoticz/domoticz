#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from pysnmp.entity.rfc3413 import config
from pysnmp.proto import rfc1905, errind
from pysnmp.proto.api import v2c
from pysnmp.proto.proxy import rfc2576
from pysnmp import error, nextid, debug
from pysnmp.proto.error import StatusInformation
from pyasn1.type import univ

getNextHandle = nextid.Integer(0x7fffffff)

__null = univ.Null('')


def getNextVarBinds(varBinds, origVarBinds=None):
    errorIndication = None
    idx = nonNulls = len(varBinds)
    rspVarBinds = []
    while idx:
        idx -= 1
        if varBinds[idx][1].tagSet in (rfc1905.NoSuchObject.tagSet,
                                       rfc1905.NoSuchInstance.tagSet,
                                       rfc1905.EndOfMibView.tagSet):
            nonNulls -= 1
        elif origVarBinds is not None:
            if v2c.ObjectIdentifier(origVarBinds[idx][0]).asTuple() >= varBinds[idx][0].asTuple():
                errorIndication = errind.oidNotIncreasing

        rspVarBinds.insert(0, (varBinds[idx][0], __null))

    if not nonNulls:
        rspVarBinds = []

    return errorIndication, rspVarBinds


class CommandGenerator(object):
    _null = univ.Null('')

    def __init__(self):
        self.__pendingReqs = {}

    def processResponsePdu(self, snmpEngine, messageProcessingModel,
                           securityModel, securityName, securityLevel,
                           contextEngineId, contextName, pduVersion,
                           PDU, statusInformation, sendPduHandle, cbCtx):
        origSendRequestHandle, cbFun, cbCtx = cbCtx

        # 3.1.1
        if sendPduHandle not in self.__pendingReqs:
            raise error.PySnmpError('Missing sendPduHandle %s' % sendPduHandle)

        (origTransportDomain, origTransportAddress,
         origMessageProcessingModel, origSecurityModel,
         origSecurityName, origSecurityLevel, origContextEngineId,
         origContextName, origPduVersion, origPdu,
         origTimeout, origRetryCount,
         origRetries) = self.__pendingReqs.pop(sendPduHandle)

        snmpEngine.transportDispatcher.jobFinished(id(self))

        # 3.1.3
        if statusInformation:
            debug.logger & debug.flagApp and debug.logger(
                'processResponsePdu: sendPduHandle %s, statusInformation %s' % (sendPduHandle, statusInformation))
            errorIndication = statusInformation['errorIndication']
            # SNMP engine discovery will take extra retries, allow that
            if (errorIndication in (errind.notInTimeWindow,
                                    errind.unknownEngineID) and
                    origRetries == origRetryCount + 2 or
                    errorIndication not in (errind.notInTimeWindow, errind.unknownEngineID) and
                    origRetries == origRetryCount):
                debug.logger & debug.flagApp and debug.logger(
                    'processResponsePdu: sendPduHandle %s, retry count %d exceeded' % (sendPduHandle, origRetries))
                cbFun(snmpEngine, origSendRequestHandle,
                      statusInformation['errorIndication'], None, cbCtx)
                return
            try:
                sendPduHandle = snmpEngine.msgAndPduDsp.sendPdu(
                    snmpEngine, origTransportDomain, origTransportAddress,
                    origMessageProcessingModel, origSecurityModel,
                    origSecurityName, origSecurityLevel, origContextEngineId,
                    origContextName, origPduVersion, origPdu,
                    True, origTimeout, self.processResponsePdu,
                    (origSendRequestHandle, cbFun, cbCtx))

                snmpEngine.transportDispatcher.jobStarted(id(self))

                self.__pendingReqs[sendPduHandle] = (
                    origTransportDomain, origTransportAddress,
                    origMessageProcessingModel, origSecurityModel,
                    origSecurityName, origSecurityLevel, origContextEngineId,
                    origContextName, origPduVersion, origPdu, origTimeout,
                    origRetryCount, origRetries + 1
                )
                return

            except StatusInformation:
                statusInformation = sys.exc_info()[1]
                debug.logger & debug.flagApp and debug.logger(
                    'processResponsePdu: origSendRequestHandle %s, _sendPdu() failed with %r' % (
                    sendPduHandle, statusInformation))
                cbFun(snmpEngine, origSendRequestHandle,
                      statusInformation['errorIndication'],
                      None, cbCtx)
                return

        if (origMessageProcessingModel != messageProcessingModel or
                origSecurityModel != securityModel or
                origSecurityName != origSecurityName or
                origContextEngineId and origContextEngineId != contextEngineId or
                origContextName and origContextName != contextName or
                origPduVersion != pduVersion):
            debug.logger & debug.flagApp and debug.logger(
                'processResponsePdu: sendPduHandle %s, request/response data mismatch' % sendPduHandle)

            cbFun(snmpEngine, origSendRequestHandle,
                  'badResponse', None, cbCtx)
            return

        # User-side API assumes SMIv2
        if messageProcessingModel == 0:
            PDU = rfc2576.v1ToV2(PDU, origPdu)

        # 3.1.2
        if v2c.apiPDU.getRequestID(PDU) != v2c.apiPDU.getRequestID(origPdu):
            debug.logger & debug.flagApp and debug.logger(
                'processResponsePdu: sendPduHandle %s, request-id/response-id mismatch' % sendPduHandle)
            cbFun(snmpEngine, origSendRequestHandle,
                  'badResponse', None, cbCtx)
            return

        cbFun(snmpEngine, origSendRequestHandle, None, PDU, cbCtx)

    def sendPdu(self, snmpEngine, targetName, contextEngineId,
                contextName, PDU, cbFun, cbCtx):
        (transportDomain, transportAddress, timeout,
         retryCount, messageProcessingModel, securityModel,
         securityName,
         securityLevel) = config.getTargetInfo(snmpEngine, targetName)

        # Convert timeout in seconds into timeout in timer ticks
        timeoutInTicks = float(timeout) / 100 / snmpEngine.transportDispatcher.getTimerResolution()

        SnmpEngineID, SnmpAdminString = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
            'SNMP-FRAMEWORK-MIB', 'SnmpEngineID', 'SnmpAdminString')

        # Cast possible strings into bytes
        if contextEngineId:
            contextEngineId = SnmpEngineID(contextEngineId)
        contextName = SnmpAdminString(contextName)

        origPDU = PDU

        # User-side API assumes SMIv2
        if messageProcessingModel == 0:
            PDU = rfc2576.v2ToV1(PDU)
            pduVersion = 0
        else:
            pduVersion = 1

        sendRequestHandle = getNextHandle()

        # 3.1
        sendPduHandle = snmpEngine.msgAndPduDsp.sendPdu(
            snmpEngine, transportDomain, transportAddress,
            messageProcessingModel, securityModel, securityName,
            securityLevel, contextEngineId, contextName,
            pduVersion, PDU, True, timeoutInTicks, self.processResponsePdu,
            (sendRequestHandle, cbFun, cbCtx)
        )

        snmpEngine.transportDispatcher.jobStarted(id(self))

        self.__pendingReqs[sendPduHandle] = (
            transportDomain, transportAddress, messageProcessingModel,
            securityModel, securityName, securityLevel, contextEngineId,
            contextName, pduVersion, origPDU, timeoutInTicks,
            retryCount, 0
        )

        debug.logger & debug.flagApp and debug.logger(
            'sendPdu: sendPduHandle %s, timeout %d*10 ms/%d ticks, retry 0 of %d' % (
                sendPduHandle, timeout, timeoutInTicks, retryCount))

        return sendRequestHandle


# backward compatibility stub
CommandGeneratorBase = CommandGenerator


class GetCommandGenerator(CommandGenerator):
    def processResponseVarBinds(self, snmpEngine, sendRequestHandle,
                                errorIndication, PDU, cbCtx):
        cbFun, cbCtx = cbCtx

        cbFun(snmpEngine, sendRequestHandle, errorIndication,
              PDU and v2c.apiPDU.getErrorStatus(PDU) or 0,
              PDU and v2c.apiPDU.getErrorIndex(PDU, muteErrors=True) or 0,
              PDU and v2c.apiPDU.getVarBinds(PDU) or (), cbCtx)

    def sendVarBinds(self, snmpEngine, targetName, contextEngineId,
                     contextName, varBinds, cbFun, cbCtx=None):
        reqPDU = v2c.GetRequestPDU()
        v2c.apiPDU.setDefaults(reqPDU)

        v2c.apiPDU.setVarBinds(reqPDU, varBinds)

        return self.sendPdu(snmpEngine, targetName, contextEngineId,
                            contextName, reqPDU, self.processResponseVarBinds,
                            (cbFun, cbCtx))


class SetCommandGenerator(CommandGenerator):
    def processResponseVarBinds(self, snmpEngine, sendRequestHandle,
                                errorIndication, PDU, cbCtx):
        cbFun, cbCtx = cbCtx

        cbFun(snmpEngine, sendRequestHandle, errorIndication,
              PDU and v2c.apiPDU.getErrorStatus(PDU) or 0,
              PDU and v2c.apiPDU.getErrorIndex(PDU, muteErrors=True) or 0,
              PDU and v2c.apiPDU.getVarBinds(PDU) or (), cbCtx)

    def sendVarBinds(self, snmpEngine, targetName, contextEngineId,
                     contextName, varBinds, cbFun, cbCtx=None):
        reqPDU = v2c.SetRequestPDU()
        v2c.apiPDU.setDefaults(reqPDU)

        v2c.apiPDU.setVarBinds(reqPDU, varBinds)

        return self.sendPdu(snmpEngine, targetName, contextEngineId,
                            contextName, reqPDU,
                            self.processResponseVarBinds, (cbFun, cbCtx))


class NextCommandGeneratorSingleRun(CommandGenerator):
    def processResponseVarBinds(self, snmpEngine, sendRequestHandle,
                                errorIndication, PDU, cbCtx):
        targetName, contextEngineId, contextName, reqPDU, cbFun, cbCtx = cbCtx

        cbFun(snmpEngine, sendRequestHandle, errorIndication,
              PDU and v2c.apiPDU.getErrorStatus(PDU) or 0,
              PDU and v2c.apiPDU.getErrorIndex(PDU, muteErrors=True) or 0,
              PDU and v2c.apiPDU.getVarBinds(PDU) or (), cbCtx)

    def sendVarBinds(self, snmpEngine, targetName, contextEngineId,
                     contextName, varBinds, cbFun, cbCtx=None):
        reqPDU = v2c.GetNextRequestPDU()
        v2c.apiPDU.setDefaults(reqPDU)

        v2c.apiPDU.setVarBinds(reqPDU, varBinds)

        return self.sendPdu(snmpEngine, targetName, contextEngineId,
                            contextName, reqPDU, self.processResponseVarBinds,
                            (targetName, contextEngineId, contextName,
                             reqPDU, cbFun, cbCtx))


class NextCommandGenerator(NextCommandGeneratorSingleRun):
    def processResponseVarBinds(self, snmpEngine, sendRequestHandle,
                                errorIndication, PDU, cbCtx):
        targetName, contextEngineId, contextName, reqPDU, cbFun, cbCtx = cbCtx

        if errorIndication:
            cbFun(snmpEngine, sendRequestHandle, errorIndication,
                  0, 0, (), cbCtx)
            return

        varBindTable = v2c.apiPDU.getVarBindTable(reqPDU, PDU)

        if v2c.apiPDU.getErrorStatus(PDU):
            errorIndication, varBinds = None, ()
        elif not varBindTable:
            errorIndication, varBinds = errind.emptyResponse, ()
        else:
            errorIndication, varBinds = getNextVarBinds(
                varBindTable[-1], v2c.apiPDU.getVarBinds(reqPDU)
            )

        if not cbFun(snmpEngine, sendRequestHandle, errorIndication,
                     v2c.apiPDU.getErrorStatus(PDU),
                     v2c.apiPDU.getErrorIndex(PDU, muteErrors=True),
                     varBindTable, cbCtx):
            debug.logger & debug.flagApp and debug.logger(
                'processResponseVarBinds: sendRequestHandle %s, app says to stop walking' % sendRequestHandle)
            return  # app says enough

        if not varBinds:
            return  # no more objects available

        v2c.apiPDU.setRequestID(reqPDU, v2c.getNextRequestID())
        v2c.apiPDU.setVarBinds(reqPDU, varBinds)

        try:
            self.sendPdu(snmpEngine, targetName, contextEngineId,
                         contextName, reqPDU,
                         self.processResponseVarBinds,
                         (targetName, contextEngineId, contextName,
                          reqPDU, cbFun, cbCtx))

        except StatusInformation:
            statusInformation = sys.exc_info()[1]
            debug.logger & debug.flagApp and debug.logger(
                'sendVarBinds: sendPduHandle %s: sendPdu() failed with %r' % (sendRequestHandle, statusInformation))
            cbFun(snmpEngine, sendRequestHandle,
                  statusInformation['errorIndication'],
                  0, 0, (), cbCtx)


class BulkCommandGeneratorSingleRun(CommandGenerator):
    def processResponseVarBinds(self, snmpEngine, sendRequestHandle,
                                errorIndication, PDU, cbCtx):
        (targetName, nonRepeaters, maxRepetitions,
         contextEngineId, contextName, reqPDU, cbFun, cbCtx) = cbCtx

        cbFun(snmpEngine, sendRequestHandle, errorIndication,
              PDU and v2c.apiPDU.getErrorStatus(PDU) or 0,
              PDU and v2c.apiPDU.getErrorIndex(PDU, muteErrors=True) or 0,
              PDU and v2c.apiPDU.getVarBinds(PDU) or (), cbCtx)

    def sendVarBinds(self, snmpEngine, targetName, contextEngineId,
                     contextName, nonRepeaters, maxRepetitions,
                     varBinds, cbFun, cbCtx=None):
        reqPDU = v2c.GetBulkRequestPDU()
        v2c.apiBulkPDU.setDefaults(reqPDU)

        v2c.apiBulkPDU.setNonRepeaters(reqPDU, nonRepeaters)
        v2c.apiBulkPDU.setMaxRepetitions(reqPDU, maxRepetitions)

        v2c.apiBulkPDU.setVarBinds(reqPDU, varBinds)

        return self.sendPdu(snmpEngine, targetName, contextEngineId,
                            contextName, reqPDU,
                            self.processResponseVarBinds,
                            (targetName, nonRepeaters, maxRepetitions,
                             contextEngineId, contextName, reqPDU,
                             cbFun, cbCtx))


class BulkCommandGenerator(BulkCommandGeneratorSingleRun):
    def processResponseVarBinds(self, snmpEngine, sendRequestHandle,
                                errorIndication, PDU, cbCtx):
        (targetName, nonRepeaters, maxRepetitions,
         contextEngineId, contextName, reqPDU, cbFun, cbCtx) = cbCtx

        if errorIndication:
            cbFun(snmpEngine, sendRequestHandle, errorIndication,
                  0, 0, (), cbCtx)
            return

        varBindTable = v2c.apiBulkPDU.getVarBindTable(reqPDU, PDU)

        if v2c.apiBulkPDU.getErrorStatus(PDU):
            errorIndication, varBinds = None, ()
        elif not varBindTable:
            errorIndication, varBinds = errind.emptyResponse, ()
        else:
            errorIndication, varBinds = getNextVarBinds(
                varBindTable[-1], v2c.apiPDU.getVarBinds(reqPDU)
            )
            nonRepeaters = v2c.apiBulkPDU.getNonRepeaters(reqPDU)
            if nonRepeaters:
                varBinds = v2c.apiBulkPDU.getVarBinds(reqPDU)[:int(nonRepeaters)] + varBinds[int(nonRepeaters):]

        if not cbFun(snmpEngine, sendRequestHandle, errorIndication,
                     v2c.apiBulkPDU.getErrorStatus(PDU),
                     v2c.apiBulkPDU.getErrorIndex(PDU, muteErrors=True),
                     varBindTable, cbCtx):
            debug.logger & debug.flagApp and debug.logger(
                'processResponseVarBinds: sendRequestHandle %s, app says to stop walking' % sendRequestHandle)
            return  # app says enough

        if not varBinds:
            return  # no more objects available

        v2c.apiBulkPDU.setRequestID(reqPDU, v2c.getNextRequestID())
        v2c.apiBulkPDU.setVarBinds(reqPDU, varBinds)

        try:
            self.sendPdu(snmpEngine, targetName, contextEngineId,
                         contextName, reqPDU,
                         self.processResponseVarBinds,
                         (targetName, nonRepeaters, maxRepetitions,
                          contextEngineId, contextName, reqPDU, cbFun, cbCtx))

        except StatusInformation:
            statusInformation = sys.exc_info()[1]
            debug.logger & debug.flagApp and debug.logger(
                'processResponseVarBinds: sendPduHandle %s: _sendPdu() failed with %r' % (
                    sendRequestHandle, statusInformation))
            cbFun(snmpEngine, sendRequestHandle,
                  statusInformation['errorIndication'], 0, 0, (), cbCtx)


#
# Obsolete, compatibility interfaces.
#

def __sendReqCbFun(snmpEngine, sendRequestHandle, errorIndication,
                   errorStatus, errorIndex, varBinds, cbCtx):
    cbFun, cbCtx = cbCtx
    return cbFun(sendRequestHandle, errorIndication, errorStatus,
                 errorIndex, varBinds, cbCtx)


def _sendReq(self, snmpEngine, targetName, varBinds, cbFun,
             cbCtx=None, contextEngineId=None, contextName=''):
    return self.sendVarBinds(snmpEngine, targetName, contextEngineId,
                             contextName, varBinds, __sendReqCbFun,
                             (cbFun, cbCtx))


def _sendBulkReq(self, snmpEngine, targetName, nonRepeaters, maxRepetitions,
                 varBinds, cbFun, cbCtx=None, contextEngineId=None,
                 contextName=''):
    return self.sendVarBinds(snmpEngine, targetName, contextEngineId,
                             contextName, nonRepeaters, maxRepetitions,
                             varBinds, __sendReqCbFun, (cbFun, cbCtx))


# install compatibility wrappers
GetCommandGenerator.sendReq = _sendReq
SetCommandGenerator.sendReq = _sendReq
NextCommandGenerator.sendReq = _sendReq
NextCommandGeneratorSingleRun.sendReq = _sendReq
BulkCommandGenerator.sendReq = _sendBulkReq
BulkCommandGeneratorSingleRun.sendReq = _sendBulkReq
