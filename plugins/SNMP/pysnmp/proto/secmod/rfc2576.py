#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from pyasn1.codec.ber import encoder
from pyasn1.error import PyAsn1Error
from pysnmp.proto.secmod import base
from pysnmp.carrier.asyncore.dgram import udp, udp6, unix
from pysnmp.smi.error import NoSuchInstanceError
from pysnmp.proto import errind, error
from pysnmp import debug


class SnmpV1SecurityModel(base.AbstractSecurityModel):
    securityModelID = 1

    # According to rfc2576, community name <-> contextEngineId/contextName
    # mapping is up to MP module for notifications but belongs to secmod
    # responsibility for other PDU types. Since I do not yet understand
    # the reason for this de-coupling, I've moved this code from MP-scope
    # in here.

    def __init__(self):
        self.__transportBranchId = self.__paramsBranchId = self.__communityBranchId = self.__securityBranchId = -1
        base.AbstractSecurityModel.__init__(self)

    def _sec2com(self, snmpEngine, securityName, contextEngineId, contextName):
        snmpTargetParamsSecurityName, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
            'SNMP-TARGET-MIB', 'snmpTargetParamsSecurityName')
        if self.__paramsBranchId != snmpTargetParamsSecurityName.branchVersionId:
            snmpTargetParamsSecurityModel, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                'SNMP-TARGET-MIB', 'snmpTargetParamsSecurityModel')

            self.__nameToModelMap = {}

            nextMibNode = snmpTargetParamsSecurityName

            while 1:
                try:
                    nextMibNode = snmpTargetParamsSecurityName.getNextNode(nextMibNode.name)

                except NoSuchInstanceError:
                    break

                instId = nextMibNode.name[len(snmpTargetParamsSecurityName.name):]

                mibNode = snmpTargetParamsSecurityModel.getNode(snmpTargetParamsSecurityModel.name + instId)

                if mibNode.syntax not in self.__nameToModelMap:
                    self.__nameToModelMap[nextMibNode.syntax] = set()

                self.__nameToModelMap[nextMibNode.syntax].add(mibNode.syntax)

            self.__paramsBranchId = snmpTargetParamsSecurityName.branchVersionId

            # invalidate next map as it include this one
            self.__securityBranchId = -1

        snmpCommunityName, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols('SNMP-COMMUNITY-MIB',
                                                                                                   'snmpCommunityName')
        if self.__securityBranchId != snmpCommunityName.branchVersionId:
            (snmpCommunitySecurityName,
             snmpCommunityContextEngineId,
             snmpCommunityContextName) = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                'SNMP-COMMUNITY-MIB', 'snmpCommunitySecurityName',
                'snmpCommunityContextEngineID', 'snmpCommunityContextName'
            )

            self.__securityMap = {}

            nextMibNode = snmpCommunityName
            while 1:
                try:
                    nextMibNode = snmpCommunityName.getNextNode(nextMibNode.name)

                except NoSuchInstanceError:
                    break

                instId = nextMibNode.name[len(snmpCommunityName.name):]

                _securityName = snmpCommunitySecurityName.getNode(snmpCommunitySecurityName.name + instId).syntax

                _contextEngineId = snmpCommunityContextEngineId.getNode(
                    snmpCommunityContextEngineId.name + instId).syntax

                _contextName = snmpCommunityContextName.getNode(snmpCommunityContextName.name + instId).syntax

                self.__securityMap[(_securityName,
                                    _contextEngineId,
                                    _contextName)] = nextMibNode.syntax

            self.__securityBranchId = snmpCommunityName.branchVersionId

            debug.logger & debug.flagSM and debug.logger(
                '_sec2com: built securityName to communityName map, version %s: %s' % (
                    self.__securityBranchId, self.__securityMap))

        try:
            return self.__securityMap[(securityName,
                                       contextEngineId,
                                       contextName)]

        except KeyError:
            raise error.StatusInformation(
                errorIndication=errind.unknownCommunityName
            )

    def _com2sec(self, snmpEngine, communityName, transportInformation):
        snmpTargetAddrTAddress, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
            'SNMP-TARGET-MIB', 'snmpTargetAddrTAddress')
        if self.__transportBranchId != snmpTargetAddrTAddress.branchVersionId:
            (SnmpTagValue, snmpTargetAddrTDomain,
             snmpTargetAddrTagList) = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                'SNMP-TARGET-MIB', 'SnmpTagValue', 'snmpTargetAddrTDomain',
                'snmpTargetAddrTagList'
            )

            self.__emptyTag = SnmpTagValue('')

            self.__transportToTagMap = {}

            nextMibNode = snmpTargetAddrTagList
            while True:
                try:
                    nextMibNode = snmpTargetAddrTagList.getNextNode(nextMibNode.name)
                except NoSuchInstanceError:
                    break
                instId = nextMibNode.name[len(snmpTargetAddrTagList.name):]
                targetAddrTDomain = snmpTargetAddrTDomain.getNode(snmpTargetAddrTDomain.name + instId).syntax
                targetAddrTAddress = snmpTargetAddrTAddress.getNode(snmpTargetAddrTAddress.name + instId).syntax

                targetAddrTDomain = tuple(targetAddrTDomain)

                if targetAddrTDomain[:len(udp.snmpUDPDomain)] == udp.snmpUDPDomain:
                    SnmpUDPAddress, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols('SNMPv2-TM',
                                                                                                            'SnmpUDPAddress')
                    targetAddrTAddress = tuple(SnmpUDPAddress(targetAddrTAddress))
                elif targetAddrTDomain[:len(udp6.snmpUDP6Domain)] == udp6.snmpUDP6Domain:
                    TransportAddressIPv6, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                        'TRANSPORT-ADDRESS-MIB', 'TransportAddressIPv6')
                    targetAddrTAddress = tuple(TransportAddressIPv6(targetAddrTAddress))
                elif targetAddrTDomain[:len(unix.snmpLocalDomain)] == unix.snmpLocalDomain:
                    targetAddrTAddress = str(targetAddrTAddress)
                targetAddr = targetAddrTDomain, targetAddrTAddress
                targetAddrTagList = snmpTargetAddrTagList.getNode(snmpTargetAddrTagList.name + instId).syntax
                if targetAddr not in self.__transportToTagMap:
                    self.__transportToTagMap[targetAddr] = set()
                if targetAddrTagList:
                    self.__transportToTagMap[targetAddr].update(
                        [SnmpTagValue(x)
                         for x in targetAddrTagList.asOctets().split()]
                    )
                else:
                    self.__transportToTagMap[targetAddr].add(self.__emptyTag)

            self.__transportBranchId = snmpTargetAddrTAddress.branchVersionId

            debug.logger & debug.flagSM and debug.logger('_com2sec: built transport-to-tag map version %s: %s' % (
                self.__transportBranchId, self.__transportToTagMap))

        snmpTargetParamsSecurityName, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
            'SNMP-TARGET-MIB', 'snmpTargetParamsSecurityName')
        if self.__paramsBranchId != snmpTargetParamsSecurityName.branchVersionId:
            snmpTargetParamsSecurityModel, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                'SNMP-TARGET-MIB', 'snmpTargetParamsSecurityModel')

            self.__nameToModelMap = {}

            nextMibNode = snmpTargetParamsSecurityName

            while True:
                try:
                    nextMibNode = snmpTargetParamsSecurityName.getNextNode(nextMibNode.name)

                except NoSuchInstanceError:
                    break

                instId = nextMibNode.name[len(snmpTargetParamsSecurityName.name):]

                mibNode = snmpTargetParamsSecurityModel.getNode(snmpTargetParamsSecurityModel.name + instId)

                if nextMibNode.syntax not in self.__nameToModelMap:
                    self.__nameToModelMap[nextMibNode.syntax] = set()

                self.__nameToModelMap[nextMibNode.syntax].add(mibNode.syntax)

            self.__paramsBranchId = snmpTargetParamsSecurityName.branchVersionId

            # invalidate next map as it include this one
            self.__communityBranchId = -1

            debug.logger & debug.flagSM and debug.logger(
                '_com2sec: built securityName to securityModel map, version %s: %s' % (
                    self.__paramsBranchId, self.__nameToModelMap))

        snmpCommunityName, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols('SNMP-COMMUNITY-MIB',
                                                                                                   'snmpCommunityName')
        if self.__communityBranchId != snmpCommunityName.branchVersionId:
            (snmpCommunitySecurityName, snmpCommunityContextEngineId,
             snmpCommunityContextName,
             snmpCommunityTransportTag) = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                'SNMP-COMMUNITY-MIB', 'snmpCommunitySecurityName',
                'snmpCommunityContextEngineID', 'snmpCommunityContextName',
                'snmpCommunityTransportTag'
            )

            self.__communityToTagMap = {}
            self.__tagAndCommunityToSecurityMap = {}

            nextMibNode = snmpCommunityName
            while True:
                try:
                    nextMibNode = snmpCommunityName.getNextNode(nextMibNode.name)

                except NoSuchInstanceError:
                    break

                instId = nextMibNode.name[len(snmpCommunityName.name):]

                securityName = snmpCommunitySecurityName.getNode(snmpCommunitySecurityName.name + instId).syntax

                contextEngineId = snmpCommunityContextEngineId.getNode(
                    snmpCommunityContextEngineId.name + instId).syntax

                contextName = snmpCommunityContextName.getNode(snmpCommunityContextName.name + instId).syntax

                transportTag = snmpCommunityTransportTag.getNode(snmpCommunityTransportTag.name + instId).syntax

                _tagAndCommunity = transportTag, nextMibNode.syntax

                if _tagAndCommunity not in self.__tagAndCommunityToSecurityMap:
                    self.__tagAndCommunityToSecurityMap[_tagAndCommunity] = set()

                self.__tagAndCommunityToSecurityMap[_tagAndCommunity].add(
                    (securityName, contextEngineId, contextName)
                )

                if nextMibNode.syntax not in self.__communityToTagMap:
                    self.__communityToTagMap[nextMibNode.syntax] = set()

                self.__communityToTagMap[nextMibNode.syntax].add(transportTag)

            self.__communityBranchId = snmpCommunityName.branchVersionId

            debug.logger & debug.flagSM and debug.logger(
                '_com2sec: built communityName to tag map (securityModel %s), version %s: %s' % (
                    self.securityModelID, self.__communityBranchId, self.__communityToTagMap))

            debug.logger & debug.flagSM and debug.logger(
                '_com2sec: built tag & community to securityName map (securityModel %s), version %s: %s' % (
                    self.securityModelID, self.__communityBranchId, self.__tagAndCommunityToSecurityMap))

        if communityName in self.__communityToTagMap:
            if transportInformation in self.__transportToTagMap:
                tags = self.__transportToTagMap[transportInformation].intersection(
                    self.__communityToTagMap[communityName])
            elif self.__emptyTag in self.__communityToTagMap[communityName]:
                tags = [self.__emptyTag]
            else:
                raise error.StatusInformation(errorIndication=errind.unknownCommunityName)

            candidateSecurityNames = []

            for x in [self.__tagAndCommunityToSecurityMap[(t, communityName)] for t in tags]:
                candidateSecurityNames.extend(list(x))

            # 5.2.1 (row selection in snmpCommunityTable)
            # Picks first match but favors entries already in targets table
            if candidateSecurityNames:
                candidateSecurityNames.sort(
                    key=lambda x, m=self.__nameToModelMap, v=self.securityModelID: (
                        not int(x[0] in m and v in m[x[0]]), str(x[0]))
                )
                chosenSecurityName = candidateSecurityNames[0]  # min()
                debug.logger & debug.flagSM and debug.logger(
                    '_com2sec: securityName candidates for communityName \'%s\' are %s; choosing securityName \'%s\'' % (
                        communityName, candidateSecurityNames, chosenSecurityName[0]))
                return chosenSecurityName

        raise error.StatusInformation(errorIndication=errind.unknownCommunityName)

    def generateRequestMsg(self, snmpEngine, messageProcessingModel,
                           globalData, maxMessageSize, securityModel,
                           securityEngineId, securityName, securityLevel,
                           scopedPDU):
        msg, = globalData
        contextEngineId, contextName, pdu = scopedPDU

        # rfc2576: 5.2.3
        communityName = self._sec2com(snmpEngine, securityName,
                                      contextEngineId, contextName)

        debug.logger & debug.flagSM and debug.logger(
            'generateRequestMsg: using community %r for securityModel %r, securityName %r, contextEngineId %r contextName %r' % (
                communityName, securityModel, securityName, contextEngineId, contextName))

        securityParameters = communityName

        msg.setComponentByPosition(1, securityParameters)
        msg.setComponentByPosition(2)
        msg.getComponentByPosition(2).setComponentByType(
            pdu.tagSet, pdu, verifyConstraints=False, matchTags=False, matchConstraints=False
        )

        debug.logger & debug.flagMP and debug.logger('generateRequestMsg: %s' % (msg.prettyPrint(),))

        try:
            return securityParameters, encoder.encode(msg)

        except PyAsn1Error:
            debug.logger & debug.flagMP and debug.logger(
                'generateRequestMsg: serialization failure: %s' % sys.exc_info()[1])
            raise error.StatusInformation(errorIndication=errind.serializationError)

    def generateResponseMsg(self, snmpEngine, messageProcessingModel,
                            globalData, maxMessageSize, securityModel,
                            securityEngineID, securityName, securityLevel,
                            scopedPDU, securityStateReference):
        # rfc2576: 5.2.2
        msg, = globalData
        contextEngineId, contextName, pdu = scopedPDU
        cachedSecurityData = self._cache.pop(securityStateReference)
        communityName = cachedSecurityData['communityName']

        debug.logger & debug.flagSM and debug.logger(
            'generateResponseMsg: recovered community %r by securityStateReference %s' % (
                communityName, securityStateReference))

        msg.setComponentByPosition(1, communityName)
        msg.setComponentByPosition(2)
        msg.getComponentByPosition(2).setComponentByType(
            pdu.tagSet, pdu, verifyConstraints=False, matchTags=False, matchConstraints=False
        )

        debug.logger & debug.flagMP and debug.logger('generateResponseMsg: %s' % (msg.prettyPrint(),))

        try:
            return communityName, encoder.encode(msg)

        except PyAsn1Error:
            debug.logger & debug.flagMP and debug.logger(
                'generateResponseMsg: serialization failure: %s' % sys.exc_info()[1])
            raise error.StatusInformation(errorIndication=errind.serializationError)

    def processIncomingMsg(self, snmpEngine, messageProcessingModel,
                           maxMessageSize, securityParameters, securityModel,
                           securityLevel, wholeMsg, msg):
        # rfc2576: 5.2.1
        communityName, transportInformation = securityParameters

        scope = dict(communityName=communityName,
                     transportInformation=transportInformation)

        snmpEngine.observer.storeExecutionContext(
            snmpEngine, 'rfc2576.processIncomingMsg:writable', scope
        )
        snmpEngine.observer.clearExecutionContext(
            snmpEngine, 'rfc2576.processIncomingMsg:writable'
        )

        try:
            securityName, contextEngineId, contextName = self._com2sec(
                snmpEngine, scope.get('communityName', communityName),
                scope.get('transportInformation', transportInformation)
            )

        except error.StatusInformation:
            snmpInBadCommunityNames, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols(
                '__SNMPv2-MIB', 'snmpInBadCommunityNames')
            snmpInBadCommunityNames.syntax += 1
            raise error.StatusInformation(
                errorIndication=errind.unknownCommunityName,
                communityName=communityName
            )

        snmpEngineID, = snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder.importSymbols('__SNMP-FRAMEWORK-MIB',
                                                                                              'snmpEngineID')

        securityEngineID = snmpEngineID.syntax

        snmpEngine.observer.storeExecutionContext(
            snmpEngine, 'rfc2576.processIncomingMsg',
            dict(transportInformation=transportInformation,
                 securityEngineId=securityEngineID,
                 securityName=securityName,
                 communityName=communityName,
                 contextEngineId=contextEngineId,
                 contextName=contextName)
        )
        snmpEngine.observer.clearExecutionContext(
            snmpEngine, 'rfc2576.processIncomingMsg'
        )

        debug.logger & debug.flagSM and debug.logger(
            'processIncomingMsg: looked up securityName %r securityModel %r contextEngineId %r contextName %r by communityName %r AND transportInformation %r' % (
                securityName, self.securityModelID, contextEngineId, contextName, communityName, transportInformation))

        stateReference = self._cache.push(communityName=communityName)

        scopedPDU = (contextEngineId, contextName,
                     msg.getComponentByPosition(2).getComponent())
        maxSizeResponseScopedPDU = maxMessageSize - 128
        securityStateReference = stateReference

        debug.logger & debug.flagSM and debug.logger(
            'processIncomingMsg: generated maxSizeResponseScopedPDU %s securityStateReference %s' % (
                maxSizeResponseScopedPDU, securityStateReference))

        return (securityEngineID, securityName, scopedPDU,
                maxSizeResponseScopedPDU, securityStateReference)


class SnmpV2cSecurityModel(SnmpV1SecurityModel):
    securityModelID = 2

# XXX
# contextEngineId/contextName goes to globalData
