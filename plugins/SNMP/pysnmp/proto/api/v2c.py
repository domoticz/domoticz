#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto import rfc1901, rfc1902, rfc1905
from pysnmp.proto.api import v1
from pyasn1.type import univ, constraint

# Shortcuts to SNMP types
Null = univ.Null
null = Null('')
ObjectIdentifier = univ.ObjectIdentifier

Integer = rfc1902.Integer
Integer32 = rfc1902.Integer32
OctetString = rfc1902.OctetString
IpAddress = rfc1902.IpAddress
Counter32 = rfc1902.Counter32
Gauge32 = rfc1902.Gauge32
Unsigned32 = rfc1902.Unsigned32
TimeTicks = rfc1902.TimeTicks
Opaque = rfc1902.Opaque
Counter64 = rfc1902.Counter64
Bits = rfc1902.Bits

NoSuchObject = rfc1905.NoSuchObject
NoSuchInstance = rfc1905.NoSuchInstance
EndOfMibView = rfc1905.EndOfMibView

VarBind = rfc1905.VarBind
VarBindList = rfc1905.VarBindList
GetRequestPDU = rfc1905.GetRequestPDU
GetNextRequestPDU = rfc1905.GetNextRequestPDU
ResponsePDU = GetResponsePDU = rfc1905.ResponsePDU
SetRequestPDU = rfc1905.SetRequestPDU
GetBulkRequestPDU = rfc1905.GetBulkRequestPDU
InformRequestPDU = rfc1905.InformRequestPDU
SNMPv2TrapPDU = TrapPDU = rfc1905.SNMPv2TrapPDU
ReportPDU = rfc1905.ReportPDU

Message = rfc1901.Message

getNextRequestID = v1.getNextRequestID

apiVarBind = v1.apiVarBind


class PDUAPI(v1.PDUAPI):
    _errorStatus = rfc1905.errorStatus.clone(0)
    _errorIndex = univ.Integer(0).subtype(subtypeSpec=constraint.ValueRangeConstraint(0, rfc1905.max_bindings))

    def getResponse(self, reqPDU):
        rspPDU = ResponsePDU()
        self.setDefaults(rspPDU)
        self.setRequestID(rspPDU, self.getRequestID(reqPDU))
        return rspPDU

    def getVarBindTable(self, reqPDU, rspPDU):
        return [apiPDU.getVarBinds(rspPDU)]

    def setEndOfMibError(self, pdu, errorIndex):
        varBindList = self.getVarBindList(pdu)
        varBindList[errorIndex - 1].setComponentByPosition(
            1, rfc1905.endOfMibView, verifyConstraints=False, matchTags=False, matchConstraints=False
        )

    def setNoSuchInstanceError(self, pdu, errorIndex):
        varBindList = self.getVarBindList(pdu)
        varBindList[errorIndex - 1].setComponentByPosition(
            1, rfc1905.noSuchInstance, verifyConstraints=False, matchTags=False, matchConstraints=False
        )


apiPDU = PDUAPI()


class BulkPDUAPI(PDUAPI):
    _nonRepeaters = rfc1905.nonRepeaters.clone(0)
    _maxRepetitions = rfc1905.maxRepetitions.clone(10)

    def setDefaults(self, pdu):
        PDUAPI.setDefaults(self, pdu)
        pdu.setComponentByPosition(
            0, getNextRequestID(), verifyConstraints=False, matchTags=False, matchConstraints=False
        )
        pdu.setComponentByPosition(
            1, self._nonRepeaters, verifyConstraints=False, matchTags=False, matchConstraints=False
        )
        pdu.setComponentByPosition(
            2, self._maxRepetitions, verifyConstraints=False, matchTags=False, matchConstraints=False
        )
        pdu.setComponentByPosition(3)

    @staticmethod
    def getNonRepeaters(pdu):
        return pdu.getComponentByPosition(1)

    @staticmethod
    def setNonRepeaters(pdu, value):
        pdu.setComponentByPosition(1, value)

    @staticmethod
    def getMaxRepetitions(pdu):
        return pdu.getComponentByPosition(2)

    @staticmethod
    def setMaxRepetitions(pdu, value):
        pdu.setComponentByPosition(2, value)

    def getVarBindTable(self, reqPDU, rspPDU):
        nonRepeaters = self.getNonRepeaters(reqPDU)

        reqVarBinds = self.getVarBinds(reqPDU)

        N = min(int(nonRepeaters), len(reqVarBinds))
        R = max(len(reqVarBinds) - N, 0)

        rspVarBinds = self.getVarBinds(rspPDU)

        varBindTable = []

        if R:
            for i in range(0, len(rspVarBinds) - N, R):
                varBindRow = rspVarBinds[:N] + rspVarBinds[N + i:N + R + i]
                # ignore stray OIDs / non-rectangular table
                if len(varBindRow) == N + R:
                    varBindTable.append(varBindRow)
        elif N:
            varBindTable.append(rspVarBinds[:N])

        return varBindTable


apiBulkPDU = BulkPDUAPI()


class TrapPDUAPI(v1.PDUAPI):
    sysUpTime = (1, 3, 6, 1, 2, 1, 1, 3, 0)
    snmpTrapAddress = (1, 3, 6, 1, 6, 3, 18, 1, 3, 0)
    snmpTrapCommunity = (1, 3, 6, 1, 6, 3, 18, 1, 4, 0)
    snmpTrapOID = (1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0)
    snmpTrapEnterprise = (1, 3, 6, 1, 6, 3, 1, 1, 4, 3, 0)
    _zeroTime = TimeTicks(0)
    _genTrap = ObjectIdentifier((1, 3, 6, 1, 6, 3, 1, 1, 5, 1))

    def setDefaults(self, pdu):
        v1.PDUAPI.setDefaults(self, pdu)
        varBinds = [(self.sysUpTime, self._zeroTime),
                    # generic trap
                    (self.snmpTrapOID, self._genTrap)]
        self.setVarBinds(pdu, varBinds)


apiTrapPDU = TrapPDUAPI()


class MessageAPI(v1.MessageAPI):
    _version = rfc1901.version.clone(1)

    def setDefaults(self, msg):
        msg.setComponentByPosition(0, self._version, verifyConstraints=False, matchTags=False, matchConstraints=False)
        msg.setComponentByPosition(1, self._community, verifyConstraints=False, matchTags=False, matchConstraints=False)
        return msg

    def getResponse(self, reqMsg):
        rspMsg = Message()
        self.setDefaults(rspMsg)
        self.setVersion(rspMsg, self.getVersion(reqMsg))
        self.setCommunity(rspMsg, self.getCommunity(reqMsg))
        self.setPDU(rspMsg, apiPDU.getResponse(self.getPDU(reqMsg)))
        return rspMsg


apiMessage = MessageAPI()
