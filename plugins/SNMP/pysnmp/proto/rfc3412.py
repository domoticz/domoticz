#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from pyasn1.compat.octets import null
from pyasn1.error import PyAsn1Error
from pysnmp.smi import builder, instrum
from pysnmp.proto import errind, error, cache
from pysnmp.proto.api import verdec  # XXX
from pysnmp.error import PySnmpError
from pysnmp import nextid, debug


class MsgAndPduDispatcher(object):
    """SNMP engine PDU & message dispatcher. Exchanges SNMP PDU's with
       applications and serialized messages with transport level.
    """

    def __init__(self, mibInstrumController=None):
        if mibInstrumController is None:
            self.mibInstrumController = instrum.MibInstrumController(
                builder.MibBuilder()
            )
        else:
            self.mibInstrumController = mibInstrumController

        self.mibInstrumController.mibBuilder.loadModules(
            'SNMPv2-MIB', 'SNMP-MPD-MIB', 'SNMP-COMMUNITY-MIB',
            'SNMP-TARGET-MIB', 'SNMP-USER-BASED-SM-MIB'
        )

        # Requests cache
        self.__cache = cache.Cache()

        # Registered context engine IDs
        self.__appsRegistration = {}

        # Source of sendPduHandle and cache of requesting apps
        self.__sendPduHandle = nextid.Integer(0xffffff)

        # To pass transport info to app (legacy)
        self.__transportInfo = {}

    # legacy
    def getTransportInfo(self, stateReference):
        if stateReference in self.__transportInfo:
            return self.__transportInfo[stateReference]
        else:
            raise error.ProtocolError(
                'No data for stateReference %s' % stateReference
            )

    # Application registration with dispatcher

    # 4.3.1
    def registerContextEngineId(self, contextEngineId, pduTypes, processPdu):
        """Register application with dispatcher"""
        # 4.3.2 -> no-op

        # 4.3.3
        for pduType in pduTypes:
            k = (contextEngineId, pduType)
            if k in self.__appsRegistration:
                raise error.ProtocolError(
                    'Duplicate registration %r/%s' % (contextEngineId, pduType)
                )

            # 4.3.4
            self.__appsRegistration[k] = processPdu

        debug.logger & debug.flagDsp and debug.logger(
            'registerContextEngineId: contextEngineId %r pduTypes %s' % (contextEngineId, pduTypes))

    # 4.4.1
    def unregisterContextEngineId(self, contextEngineId, pduTypes):
        """Unregister application with dispatcher"""
        # 4.3.4
        if contextEngineId is None:
            # Default to local snmpEngineId
            contextEngineId, = self.mibInstrumController.mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB',
                                                                                  'snmpEngineID')

        for pduType in pduTypes:
            k = (contextEngineId, pduType)
            if k in self.__appsRegistration:
                del self.__appsRegistration[k]

        debug.logger & debug.flagDsp and debug.logger(
            'unregisterContextEngineId: contextEngineId %r pduTypes %s' % (contextEngineId, pduTypes))

    def getRegisteredApp(self, contextEngineId, pduType):
        k = (contextEngineId, pduType)
        if k in self.__appsRegistration:
            return self.__appsRegistration[k]
        k = (null, pduType)
        if k in self.__appsRegistration:
            return self.__appsRegistration[k]  # wildcard

    # Dispatcher <-> application API

    # 4.1.1

    def sendPdu(self, snmpEngine, transportDomain, transportAddress,
                messageProcessingModel, securityModel, securityName,
                securityLevel, contextEngineId, contextName,
                pduVersion, PDU, expectResponse, timeout=0,
                cbFun=None, cbCtx=None):
        """PDU dispatcher -- prepare and serialize a request or notification"""
        # 4.1.1.2
        k = int(messageProcessingModel)
        if k in snmpEngine.messageProcessingSubsystems:
            mpHandler = snmpEngine.messageProcessingSubsystems[k]
        else:
            raise error.StatusInformation(
                errorIndication=errind.unsupportedMsgProcessingModel
            )

        debug.logger & debug.flagDsp and debug.logger(
            'sendPdu: securityName %s, PDU\n%s' % (securityName, PDU.prettyPrint()))

        # 4.1.1.3
        sendPduHandle = self.__sendPduHandle()
        if expectResponse:
            self.__cache.add(
                sendPduHandle,
                messageProcessingModel=messageProcessingModel,
                sendPduHandle=sendPduHandle,
                timeout=timeout + snmpEngine.transportDispatcher.getTimerTicks(),
                cbFun=cbFun,
                cbCtx=cbCtx
            )

            debug.logger & debug.flagDsp and debug.logger('sendPdu: current time %d ticks, one tick is %s seconds' % (
                snmpEngine.transportDispatcher.getTimerTicks(), snmpEngine.transportDispatcher.getTimerResolution()))

        debug.logger & debug.flagDsp and debug.logger(
            'sendPdu: new sendPduHandle %s, timeout %s ticks, cbFun %s' % (sendPduHandle, timeout, cbFun))

        origTransportDomain = transportDomain
        origTransportAddress = transportAddress

        # 4.1.1.4 & 4.1.1.5
        try:
            (transportDomain,
             transportAddress,
             outgoingMessage) = mpHandler.prepareOutgoingMessage(
                snmpEngine, origTransportDomain, origTransportAddress,
                messageProcessingModel, securityModel, securityName,
                securityLevel, contextEngineId, contextName,
                pduVersion, PDU, expectResponse, sendPduHandle
            )

            debug.logger & debug.flagDsp and debug.logger('sendPdu: MP succeeded')
        except PySnmpError:
            if expectResponse:
                self.__cache.pop(sendPduHandle)
                self.releaseStateInformation(snmpEngine, sendPduHandle, messageProcessingModel)
            raise

        # 4.1.1.6
        if snmpEngine.transportDispatcher is None:
            if expectResponse:
                self.__cache.pop(sendPduHandle)

            raise error.PySnmpError('Transport dispatcher not set')

        snmpEngine.observer.storeExecutionContext(
            snmpEngine, 'rfc3412.sendPdu',
            dict(transportDomain=transportDomain,
                 transportAddress=transportAddress,
                 outgoingMessage=outgoingMessage,
                 messageProcessingModel=messageProcessingModel,
                 securityModel=securityModel,
                 securityName=securityName,
                 securityLevel=securityLevel,
                 contextEngineId=contextEngineId,
                 contextName=contextName,
                 pdu=PDU)
        )

        try:
            snmpEngine.transportDispatcher.sendMessage(
                outgoingMessage, transportDomain, transportAddress
            )
        except PySnmpError:
            if expectResponse:
                self.__cache.pop(sendPduHandle)
            raise

        snmpEngine.observer.clearExecutionContext(snmpEngine, 'rfc3412.sendPdu')

        # Update cache with orignal req params (used for retrying)
        if expectResponse:
            self.__cache.update(sendPduHandle,
                                transportDomain=origTransportDomain,
                                transportAddress=origTransportAddress,
                                securityModel=securityModel,
                                securityName=securityName,
                                securityLevel=securityLevel,
                                contextEngineId=contextEngineId,
                                contextName=contextName,
                                pduVersion=pduVersion,
                                PDU=PDU)

        return sendPduHandle

    # 4.1.2.1
    def returnResponsePdu(self, snmpEngine, messageProcessingModel,
                          securityModel, securityName, securityLevel,
                          contextEngineId, contextName, pduVersion,
                          PDU, maxSizeResponseScopedPDU, stateReference,
                          statusInformation):
        # Extract input values and initialize defaults
        k = int(messageProcessingModel)
        if k in snmpEngine.messageProcessingSubsystems:
            mpHandler = snmpEngine.messageProcessingSubsystems[k]
        else:
            raise error.StatusInformation(
                errorIndication=errind.unsupportedMsgProcessingModel
            )

        debug.logger & debug.flagDsp and debug.logger(
            'returnResponsePdu: PDU %s' % (PDU and PDU.prettyPrint() or "<empty>",))

        # 4.1.2.2
        try:
            (transportDomain,
             transportAddress,
             outgoingMessage) = mpHandler.prepareResponseMessage(
                snmpEngine, messageProcessingModel, securityModel,
                securityName, securityLevel, contextEngineId, contextName,
                pduVersion, PDU, maxSizeResponseScopedPDU, stateReference,
                statusInformation
            )

            debug.logger & debug.flagDsp and debug.logger('returnResponsePdu: MP suceeded')

        except error.StatusInformation:
            # 4.1.2.3
            raise

        # Handle oversized messages XXX transport constrains?
        snmpEngineMaxMessageSize, = self.mibInstrumController.mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB',
                                                                                       'snmpEngineMaxMessageSize')
        if (snmpEngineMaxMessageSize.syntax and
                len(outgoingMessage) > snmpEngineMaxMessageSize.syntax):
            snmpSilentDrops, = self.mibInstrumController.mibBuilder.importSymbols('__SNMPv2-MIB', 'snmpSilentDrops')
            snmpSilentDrops.syntax += 1
            raise error.StatusInformation(errorIndication=errind.tooBig)

        snmpEngine.observer.storeExecutionContext(
            snmpEngine,
            'rfc3412.returnResponsePdu',
            dict(transportDomain=transportDomain,
                 transportAddress=transportAddress,
                 outgoingMessage=outgoingMessage,
                 messageProcessingModel=messageProcessingModel,
                 securityModel=securityModel,
                 securityName=securityName,
                 securityLevel=securityLevel,
                 contextEngineId=contextEngineId,
                 contextName=contextName,
                 pdu=PDU)
        )

        # 4.1.2.4
        snmpEngine.transportDispatcher.sendMessage(outgoingMessage,
                                                   transportDomain,
                                                   transportAddress)

        snmpEngine.observer.clearExecutionContext(
            snmpEngine, 'rfc3412.returnResponsePdu'
        )

    # 4.2.1
    def receiveMessage(self, snmpEngine, transportDomain,
                       transportAddress, wholeMsg):
        """Message dispatcher -- de-serialize message into PDU"""
        # 4.2.1.1
        snmpInPkts, = self.mibInstrumController.mibBuilder.importSymbols(
            '__SNMPv2-MIB', 'snmpInPkts'
        )
        snmpInPkts.syntax += 1

        # 4.2.1.2
        try:
            restOfWholeMsg = null  # XXX fix decoder non-recursive return
            msgVersion = verdec.decodeMessageVersion(wholeMsg)

        except error.ProtocolError:
            snmpInASNParseErrs, = self.mibInstrumController.mibBuilder.importSymbols('__SNMPv2-MIB',
                                                                                     'snmpInASNParseErrs')
            snmpInASNParseErrs.syntax += 1
            return null  # n.b the whole buffer gets dropped

        debug.logger & debug.flagDsp and debug.logger('receiveMessage: msgVersion %s, msg decoded' % msgVersion)

        messageProcessingModel = msgVersion

        try:
            mpHandler = snmpEngine.messageProcessingSubsystems[int(messageProcessingModel)]

        except KeyError:
            snmpInBadVersions, = self.mibInstrumController.mibBuilder.importSymbols('__SNMPv2-MIB', 'snmpInBadVersions')
            snmpInBadVersions.syntax += 1
            return restOfWholeMsg

        # 4.2.1.3 -- no-op

        # 4.2.1.4
        try:
            (messageProcessingModel,
             securityModel,
             securityName,
             securityLevel,
             contextEngineId,
             contextName,
             pduVersion,
             PDU,
             pduType,
             sendPduHandle,
             maxSizeResponseScopedPDU,
             statusInformation,
             stateReference) = mpHandler.prepareDataElements(
                snmpEngine, transportDomain, transportAddress, wholeMsg
            )

            debug.logger & debug.flagDsp and debug.logger('receiveMessage: MP succeded')

        except error.StatusInformation:
            statusInformation = sys.exc_info()[1]
            if 'sendPduHandle' in statusInformation:
                # Dropped REPORT -- re-run pending reqs queue as some
                # of them may be waiting for this REPORT
                debug.logger & debug.flagDsp and debug.logger(
                    'receiveMessage: MP failed, statusInformation %s, forcing a retry' % statusInformation)
                self.__expireRequest(
                    statusInformation['sendPduHandle'],
                    self.__cache.pop(statusInformation['sendPduHandle']),
                    snmpEngine,
                    statusInformation
                )
            return restOfWholeMsg

        except PyAsn1Error:
            debug.logger & debug.flagMP and debug.logger('receiveMessage: %s' % (sys.exc_info()[1],))
            snmpInASNParseErrs, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols('__SNMPv2-MIB', 'snmpInASNParseErrs')
            snmpInASNParseErrs.syntax += 1

            return restOfWholeMsg

        debug.logger & debug.flagDsp and debug.logger('receiveMessage: PDU %s' % PDU.prettyPrint())

        # 4.2.2
        if sendPduHandle is None:
            # 4.2.2.1 (request or notification)

            debug.logger & debug.flagDsp and debug.logger('receiveMessage: pduType %s' % pduType)
            # 4.2.2.1.1
            processPdu = self.getRegisteredApp(contextEngineId, pduType)

            # 4.2.2.1.2
            if processPdu is None:
                # 4.2.2.1.2.a
                snmpUnknownPDUHandlers, = self.mibInstrumController.mibBuilder.importSymbols('__SNMP-MPD-MIB',
                                                                                             'snmpUnknownPDUHandlers')
                snmpUnknownPDUHandlers.syntax += 1

                # 4.2.2.1.2.b
                statusInformation = {
                    'errorIndication': errind.unknownPDUHandler,
                    'oid': snmpUnknownPDUHandlers.name,
                    'val': snmpUnknownPDUHandlers.syntax
                }

                debug.logger & debug.flagDsp and debug.logger('receiveMessage: unhandled PDU type')

                # 4.2.2.1.2.c
                try:
                    (destTransportDomain,
                     destTransportAddress,
                     outgoingMessage) = mpHandler.prepareResponseMessage(
                        snmpEngine, messageProcessingModel,
                        securityModel, securityName, securityLevel,
                        contextEngineId, contextName, pduVersion,
                        PDU, maxSizeResponseScopedPDU, stateReference,
                        statusInformation
                    )

                    snmpEngine.transportDispatcher.sendMessage(
                        outgoingMessage, destTransportDomain,
                        destTransportAddress
                    )

                except PySnmpError:
                    debug.logger & debug.flagDsp and debug.logger(
                        'receiveMessage: report failed, statusInformation %s' % sys.exc_info()[1])

                else:
                    debug.logger & debug.flagDsp and debug.logger('receiveMessage: reporting succeeded')

                # 4.2.2.1.2.d
                return restOfWholeMsg

            else:
                snmpEngine.observer.storeExecutionContext(
                    snmpEngine, 'rfc3412.receiveMessage:request',
                    dict(transportDomain=transportDomain,
                         transportAddress=transportAddress,
                         wholeMsg=wholeMsg,
                         messageProcessingModel=messageProcessingModel,
                         securityModel=securityModel,
                         securityName=securityName,
                         securityLevel=securityLevel,
                         contextEngineId=contextEngineId,
                         contextName=contextName,
                         pdu=PDU)
                )

                # pass transport info to app (legacy)
                if stateReference is not None:
                    self.__transportInfo[stateReference] = (
                        transportDomain, transportAddress
                    )

                # 4.2.2.1.3
                processPdu(snmpEngine, messageProcessingModel,
                           securityModel, securityName, securityLevel,
                           contextEngineId, contextName, pduVersion,
                           PDU, maxSizeResponseScopedPDU, stateReference)

                snmpEngine.observer.clearExecutionContext(
                    snmpEngine, 'rfc3412.receiveMessage:request'
                )

                # legacy
                if stateReference is not None:
                    del self.__transportInfo[stateReference]

                debug.logger & debug.flagDsp and debug.logger('receiveMessage: processPdu succeeded')
                return restOfWholeMsg
        else:
            # 4.2.2.2 (response)

            # 4.2.2.2.1
            cachedParams = self.__cache.pop(sendPduHandle)

            # 4.2.2.2.2
            if cachedParams is None:
                snmpUnknownPDUHandlers, = self.mibInstrumController.mibBuilder.importSymbols('__SNMP-MPD-MIB',
                                                                                             'snmpUnknownPDUHandlers')
                snmpUnknownPDUHandlers.syntax += 1
                return restOfWholeMsg

            debug.logger & debug.flagDsp and debug.logger(
                'receiveMessage: cache read by sendPduHandle %s' % sendPduHandle)

            # 4.2.2.2.3
            # no-op ? XXX

            snmpEngine.observer.storeExecutionContext(
                snmpEngine, 'rfc3412.receiveMessage:response',
                dict(transportDomain=transportDomain,
                     transportAddress=transportAddress,
                     wholeMsg=wholeMsg,
                     messageProcessingModel=messageProcessingModel,
                     securityModel=securityModel,
                     securityName=securityName,
                     securityLevel=securityLevel,
                     contextEngineId=contextEngineId,
                     contextName=contextName,
                     pdu=PDU)
            )

            # 4.2.2.2.4
            processResponsePdu = cachedParams['cbFun']

            processResponsePdu(snmpEngine, messageProcessingModel,
                               securityModel, securityName, securityLevel,
                               contextEngineId, contextName, pduVersion,
                               PDU, statusInformation,
                               cachedParams['sendPduHandle'],
                               cachedParams['cbCtx'])

            snmpEngine.observer.clearExecutionContext(
                snmpEngine, 'rfc3412.receiveMessage:response'
            )

            debug.logger & debug.flagDsp and debug.logger('receiveMessage: processResponsePdu succeeded')

            return restOfWholeMsg

    def releaseStateInformation(self, snmpEngine, sendPduHandle,
                                messageProcessingModel):
        k = int(messageProcessingModel)
        if k in snmpEngine.messageProcessingSubsystems:
            mpHandler = snmpEngine.messageProcessingSubsystems[k]
            mpHandler.releaseStateInformation(sendPduHandle)

        self.__cache.pop(sendPduHandle)

    # Cache expiration stuff

    # noinspection PyUnusedLocal
    def __expireRequest(self, cacheKey, cachedParams, snmpEngine,
                        statusInformation=None):
        timeNow = snmpEngine.transportDispatcher.getTimerTicks()
        timeoutAt = cachedParams['timeout']

        if statusInformation is None and timeNow < timeoutAt:
            return

        processResponsePdu = cachedParams['cbFun']

        debug.logger & debug.flagDsp and debug.logger('__expireRequest: req cachedParams %s' % cachedParams)

        # Fail timed-out requests
        if not statusInformation:
            statusInformation = error.StatusInformation(
                errorIndication=errind.requestTimedOut
            )

        self.releaseStateInformation(snmpEngine,
                                     cachedParams['sendPduHandle'],
                                     cachedParams['messageProcessingModel'])

        processResponsePdu(snmpEngine, None, None, None, None, None,
                           None, None, None, statusInformation,
                           cachedParams['sendPduHandle'],
                           cachedParams['cbCtx'])
        return True

    # noinspection PyUnusedLocal
    def receiveTimerTick(self, snmpEngine, timeNow):
        self.__cache.expire(self.__expireRequest, snmpEngine)
