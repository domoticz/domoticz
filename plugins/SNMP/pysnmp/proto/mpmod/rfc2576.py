#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from pyasn1.codec.ber import decoder, eoo
from pyasn1.type import univ
from pyasn1.compat.octets import null
from pyasn1.error import PyAsn1Error
from pysnmp.proto.mpmod.base import AbstractMessageProcessingModel
from pysnmp.proto import rfc3411, errind, error
from pysnmp.proto.api import v1, v2c
from pysnmp import debug


# Since I have not found a detailed reference to v1MP/v2cMP
# inner workings, the following has been patterned from v3MP. Most
# references here goes to RFC3412.

class SnmpV1MessageProcessingModel(AbstractMessageProcessingModel):
    messageProcessingModelID = univ.Integer(0)  # SNMPv1
    snmpMsgSpec = v1.Message

    # rfc3412: 7.1
    def prepareOutgoingMessage(self, snmpEngine, transportDomain,
                               transportAddress, messageProcessingModel,
                               securityModel, securityName, securityLevel,
                               contextEngineId, contextName, pduVersion,
                               pdu, expectResponse, sendPduHandle):
        mibBuilder = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder

        snmpEngineId, = mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB', 'snmpEngineID')
        snmpEngineId = snmpEngineId.syntax

        # rfc3412: 7.1.1b
        if pdu.tagSet in rfc3411.confirmedClassPDUs:
            # serve unique PDU request-id
            msgID = self._cache.newMsgID()
            reqID = pdu.getComponentByPosition(0)
            debug.logger & debug.flagMP and debug.logger(
                'prepareOutgoingMessage: PDU request-id %s replaced with unique ID %s' % (reqID, msgID))

        # rfc3412: 7.1.4
        # Since there's no SNMP engine identification in v1/2c,
        # set destination contextEngineId to ours
        if not contextEngineId:
            contextEngineId = snmpEngineId

        # rfc3412: 7.1.5
        if not contextName:
            contextName = null

        debug.logger & debug.flagMP and debug.logger(
            'prepareOutgoingMessage: using contextEngineId %r contextName %r' % (contextEngineId, contextName))

        # rfc3412: 7.1.6
        scopedPDU = (contextEngineId, contextName, pdu)

        msg = self._snmpMsgSpec
        msg.setComponentByPosition(0, self.messageProcessingModelID)
        msg.setComponentByPosition(2)
        msg.getComponentByPosition(2).setComponentByType(
            pdu.tagSet, pdu, verifyConstraints=False, matchTags=False, matchConstraints=False
        )

        # rfc3412: 7.1.7
        globalData = (msg,)

        k = int(securityModel)
        if k in snmpEngine.securityModels:
            smHandler = snmpEngine.securityModels[k]
        else:
            raise error.StatusInformation(
                errorIndication=errind.unsupportedSecurityModel
            )

        # rfc3412: 7.1.9.a & rfc2576: 5.2.1 --> no-op

        snmpEngineMaxMessageSize, = mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB',
                                                             'snmpEngineMaxMessageSize')

        # fix unique request-id right prior PDU serialization
        if pdu.tagSet in rfc3411.confirmedClassPDUs:
            # noinspection PyUnboundLocalVariable
            pdu.setComponentByPosition(0, msgID)

        # rfc3412: 7.1.9.b
        (securityParameters,
         wholeMsg) = smHandler.generateRequestMsg(
            snmpEngine, self.messageProcessingModelID, globalData,
            snmpEngineMaxMessageSize.syntax, securityModel,
            snmpEngineId, securityName, securityLevel, scopedPDU
        )

        # return original request-id right after PDU serialization
        if pdu.tagSet in rfc3411.confirmedClassPDUs:
            # noinspection PyUnboundLocalVariable
            pdu.setComponentByPosition(0, reqID)

        # rfc3412: 7.1.9.c
        if pdu.tagSet in rfc3411.confirmedClassPDUs:
            # XXX rfc bug? why stateReference should be created?
            self._cache.pushByMsgId(int(msgID), sendPduHandle=sendPduHandle,
                                    reqID=reqID, snmpEngineId=snmpEngineId,
                                    securityModel=securityModel,
                                    securityName=securityName,
                                    securityLevel=securityLevel,
                                    contextEngineId=contextEngineId,
                                    contextName=contextName,
                                    transportDomain=transportDomain,
                                    transportAddress=transportAddress)

        communityName = msg.getComponentByPosition(1)  # for observer

        snmpEngine.observer.storeExecutionContext(
            snmpEngine, 'rfc2576.prepareOutgoingMessage',
            dict(transportDomain=transportDomain,
                 transportAddress=transportAddress,
                 wholeMsg=wholeMsg,
                 securityModel=securityModel,
                 securityName=securityName,
                 securityLevel=securityLevel,
                 contextEngineId=contextEngineId,
                 contextName=contextName,
                 communityName=communityName,
                 pdu=pdu)
        )
        snmpEngine.observer.clearExecutionContext(
            snmpEngine, 'rfc2576.prepareOutgoingMessage'
        )

        return transportDomain, transportAddress, wholeMsg

    # rfc3412: 7.1
    def prepareResponseMessage(self, snmpEngine, messageProcessingModel,
                               securityModel, securityName, securityLevel,
                               contextEngineId, contextName, pduVersion,
                               pdu, maxSizeResponseScopedPDU, stateReference,
                               statusInformation):
        mibBuilder = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder

        snmpEngineId, = mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB', 'snmpEngineID')
        snmpEngineId = snmpEngineId.syntax

        # rfc3412: 7.1.2.b
        if stateReference is None:
            raise error.StatusInformation(errorIndication=errind.nonReportable)

        cachedParams = self._cache.popByStateRef(stateReference)
        msgID = cachedParams['msgID']
        reqID = cachedParams['reqID']
        contextEngineId = cachedParams['contextEngineId']
        contextName = cachedParams['contextName']
        securityModel = cachedParams['securityModel']
        securityName = cachedParams['securityName']
        securityLevel = cachedParams['securityLevel']
        securityStateReference = cachedParams['securityStateReference']
        maxMessageSize = cachedParams['msgMaxSize']
        transportDomain = cachedParams['transportDomain']
        transportAddress = cachedParams['transportAddress']

        debug.logger & debug.flagMP and debug.logger(
            'prepareResponseMessage: cache read msgID %s transportDomain %s transportAddress %s by stateReference %s' % (
                msgID, transportDomain, transportAddress, stateReference))

        # rfc3412: 7.1.3
        if statusInformation:
            # rfc3412: 7.1.3a (N/A)

            # rfc3412: 7.1.3b (always discard)
            raise error.StatusInformation(errorIndication=errind.nonReportable)

        # rfc3412: 7.1.4
        # Since there's no SNMP engine identification in v1/2c,
        # set destination contextEngineId to ours
        if not contextEngineId:
            contextEngineId = snmpEngineId

        # rfc3412: 7.1.5
        if not contextName:
            contextName = null

        # rfc3412: 7.1.6
        scopedPDU = (contextEngineId, contextName, pdu)

        debug.logger & debug.flagMP and debug.logger(
            'prepareResponseMessage: using contextEngineId %r contextName %r' % (contextEngineId, contextName))

        msg = self._snmpMsgSpec
        msg.setComponentByPosition(0, messageProcessingModel)
        msg.setComponentByPosition(2)
        msg.getComponentByPosition(2).setComponentByType(
            pdu.tagSet, pdu, verifyConstraints=False, matchTags=False, matchConstraints=False
        )

        # att: msgId not set back to PDU as it's up to responder app

        # rfc3412: 7.1.7
        globalData = (msg,)

        k = int(securityModel)
        if k in snmpEngine.securityModels:
            smHandler = snmpEngine.securityModels[k]
        else:
            raise error.StatusInformation(
                errorIndication=errind.unsupportedSecurityModel
            )

        # set original request-id right prior to PDU serialization
        pdu.setComponentByPosition(0, reqID)

        # rfc3412: 7.1.8.a
        (securityParameters, wholeMsg) = smHandler.generateResponseMsg(
            snmpEngine, self.messageProcessingModelID, globalData,
            maxMessageSize, securityModel, snmpEngineId, securityName,
            securityLevel, scopedPDU, securityStateReference
        )

        # recover unique request-id right after PDU serialization
        pdu.setComponentByPosition(0, msgID)

        snmpEngine.observer.storeExecutionContext(
            snmpEngine, 'rfc2576.prepareResponseMessage',
            dict(transportDomain=transportDomain,
                 transportAddress=transportAddress,
                 securityModel=securityModel,
                 securityName=securityName,
                 securityLevel=securityLevel,
                 contextEngineId=contextEngineId,
                 contextName=contextName,
                 securityEngineId=snmpEngineId,
                 communityName=msg.getComponentByPosition(1),
                 pdu=pdu)
        )
        snmpEngine.observer.clearExecutionContext(
            snmpEngine, 'rfc2576.prepareResponseMessage'
        )

        return transportDomain, transportAddress, wholeMsg

    # rfc3412: 7.2.1

    def prepareDataElements(self, snmpEngine, transportDomain,
                            transportAddress, wholeMsg):
        mibBuilder = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder

        # rfc3412: 7.2.2
        msg, restOfWholeMsg = decoder.decode(wholeMsg, asn1Spec=self._snmpMsgSpec)

        debug.logger & debug.flagMP and debug.logger('prepareDataElements: %s' % (msg.prettyPrint(),))

        if eoo.endOfOctets.isSameTypeWith(msg):
            raise error.StatusInformation(errorIndication=errind.parseError)

        # rfc3412: 7.2.3
        msgVersion = msg.getComponentByPosition(0)

        # rfc2576: 5.2.1
        snmpEngineMaxMessageSize, = mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB', 'snmpEngineMaxMessageSize')
        communityName = msg.getComponentByPosition(1)
        # transportDomain identifies local endpoint
        securityParameters = (communityName, (transportDomain, transportAddress))
        messageProcessingModel = int(msg.getComponentByPosition(0))
        securityModel = messageProcessingModel + 1
        securityLevel = 1

        # rfc3412: 7.2.4 -- 7.2.5 -> no-op

        try:

            try:
                smHandler = snmpEngine.securityModels[int(securityModel)]

            except KeyError:
                raise error.StatusInformation(
                    errorIndication=errind.unsupportedSecurityModel
                )

            # rfc3412: 7.2.6
            (securityEngineId,
             securityName,
             scopedPDU,
             maxSizeResponseScopedPDU,
             securityStateReference) = smHandler.processIncomingMsg(
                snmpEngine, messageProcessingModel,
                snmpEngineMaxMessageSize.syntax, securityParameters,
                securityModel, securityLevel, wholeMsg, msg
            )

            debug.logger & debug.flagMP and debug.logger(
                'prepareDataElements: SM returned securityEngineId %r securityName %r' % (securityEngineId, securityName))

        except error.StatusInformation:
            statusInformation = sys.exc_info()[1]

            snmpEngine.observer.storeExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:sm-failure',
                dict(transportDomain=transportDomain,
                     transportAddress=transportAddress,
                     securityModel=securityModel,
                     securityLevel=securityLevel,
                     securityParameters=securityParameters,
                     statusInformation=statusInformation)
            )
            snmpEngine.observer.clearExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:sm-failure'
            )

            raise

        # rfc3412: 7.2.6a --> no-op

        # rfc3412: 7.2.7
        contextEngineId, contextName, pdu = scopedPDU

        # rfc2576: 5.2.1
        pduVersion = msgVersion
        pduType = pdu.tagSet

        # rfc3412: 7.2.8, 7.2.9 -> no-op

        # rfc3412: 7.2.10
        if pduType in rfc3411.responseClassPDUs:
            # get unique PDU request-id
            msgID = pdu.getComponentByPosition(0)

            # 7.2.10a
            try:
                cachedReqParams = self._cache.popByMsgId(int(msgID))
            except error.ProtocolError:
                smHandler.releaseStateInformation(securityStateReference)
                raise error.StatusInformation(errorIndication=errind.dataMismatch)

            # recover original PDU request-id to return to app
            pdu.setComponentByPosition(0, cachedReqParams['reqID'])

            debug.logger & debug.flagMP and debug.logger(
                'prepareDataElements: unique PDU request-id %s replaced with original ID %s' % (
                    msgID, cachedReqParams['reqID']))

            # 7.2.10b
            sendPduHandle = cachedReqParams['sendPduHandle']
        else:
            sendPduHandle = None

        # no error by default
        statusInformation = None

        # rfc3412: 7.2.11 -> no-op

        # rfc3412: 7.2.12
        if pduType in rfc3411.responseClassPDUs:
            # rfc3412: 7.2.12a -> no-op
            # rfc3412: 7.2.12b
            # noinspection PyUnboundLocalVariable
            if (securityModel != cachedReqParams['securityModel'] or
                    securityName != cachedReqParams['securityName'] or
                    securityLevel != cachedReqParams['securityLevel'] or
                    contextEngineId != cachedReqParams['contextEngineId'] or
                    contextName != cachedReqParams['contextName']):
                smHandler.releaseStateInformation(securityStateReference)
                raise error.StatusInformation(errorIndication=errind.dataMismatch)

            stateReference = None

            snmpEngine.observer.storeExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:response',
                dict(transportDomain=transportDomain,
                     transportAddress=transportAddress,
                     securityModel=securityModel,
                     securityName=securityName,
                     securityLevel=securityLevel,
                     contextEngineId=contextEngineId,
                     contextName=contextName,
                     securityEngineId=securityEngineId,
                     communityName=communityName,
                     pdu=pdu)
            )
            snmpEngine.observer.clearExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:response'
            )

            # rfc3412: 7.2.12c
            smHandler.releaseStateInformation(securityStateReference)

            # rfc3412: 7.2.12d
            return (messageProcessingModel, securityModel,
                    securityName, securityLevel, contextEngineId,
                    contextName, pduVersion, pdu, pduType, sendPduHandle,
                    maxSizeResponseScopedPDU, statusInformation,
                    stateReference)

        # rfc3412: 7.2.13
        if pduType in rfc3411.confirmedClassPDUs:
            # store original PDU request-id and replace it with a unique one
            reqID = pdu.getComponentByPosition(0)
            msgID = self._cache.newMsgID()
            pdu.setComponentByPosition(0, msgID)
            debug.logger & debug.flagMP and debug.logger(
                'prepareDataElements: received PDU request-id %s replaced with unique ID %s' % (reqID, msgID))

            # rfc3412: 7.2.13a
            snmpEngineId, = mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB', 'snmpEngineID')
            if securityEngineId != snmpEngineId.syntax:
                smHandler.releaseStateInformation(securityStateReference)
                raise error.StatusInformation(
                    errorIndication=errind.engineIDMismatch
                )

            # rfc3412: 7.2.13b
            stateReference = self._cache.newStateReference()
            self._cache.pushByStateRef(
                stateReference, msgVersion=messageProcessingModel,
                msgID=msgID, reqID=reqID, contextEngineId=contextEngineId,
                contextName=contextName, securityModel=securityModel,
                securityName=securityName, securityLevel=securityLevel,
                securityStateReference=securityStateReference,
                msgMaxSize=snmpEngineMaxMessageSize.syntax,
                maxSizeResponseScopedPDU=maxSizeResponseScopedPDU,
                transportDomain=transportDomain,
                transportAddress=transportAddress
            )

            snmpEngine.observer.storeExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:confirmed',
                dict(transportDomain=transportDomain,
                     transportAddress=transportAddress,
                     securityModel=securityModel,
                     securityName=securityName,
                     securityLevel=securityLevel,
                     contextEngineId=contextEngineId,
                     contextName=contextName,
                     securityEngineId=securityEngineId,
                     communityName=communityName,
                     pdu=pdu)
            )
            snmpEngine.observer.clearExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:confirmed'
            )

            debug.logger & debug.flagMP and debug.logger(
                'prepareDataElements: cached by new stateReference %s' % stateReference)

            # rfc3412: 7.2.13c
            return (messageProcessingModel, securityModel, securityName,
                    securityLevel, contextEngineId, contextName,
                    pduVersion, pdu, pduType, sendPduHandle,
                    maxSizeResponseScopedPDU, statusInformation,
                    stateReference)

        # rfc3412: 7.2.14
        if pduType in rfc3411.unconfirmedClassPDUs:
            # Pass new stateReference to let app browse request details
            stateReference = self._cache.newStateReference()

            snmpEngine.observer.storeExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:unconfirmed',
                dict(transportDomain=transportDomain,
                     transportAddress=transportAddress,
                     securityModel=securityModel,
                     securityName=securityName,
                     securityLevel=securityLevel,
                     contextEngineId=contextEngineId,
                     contextName=contextName,
                     securityEngineId=securityEngineId,
                     communityName=communityName,
                     pdu=pdu)
            )
            snmpEngine.observer.clearExecutionContext(
                snmpEngine, 'rfc2576.prepareDataElements:unconfirmed'
            )

            # This is not specified explicitly in RFC
            smHandler.releaseStateInformation(securityStateReference)

            return (messageProcessingModel, securityModel, securityName,
                    securityLevel, contextEngineId, contextName,
                    pduVersion, pdu, pduType, sendPduHandle,
                    maxSizeResponseScopedPDU, statusInformation,
                    stateReference)

        smHandler.releaseStateInformation(securityStateReference)
        raise error.StatusInformation(errorIndication=errind.unsupportedPDUtype)


class SnmpV2cMessageProcessingModel(SnmpV1MessageProcessingModel):
    messageProcessingModelID = univ.Integer(1)  # SNMPv2c
    snmpMsgSpec = v2c.Message
