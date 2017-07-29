#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pyasn1.type import univ
from pysnmp.proto import rfc1155, rfc1157, error
from pysnmp import nextid

# Shortcuts to SNMP types
Integer = univ.Integer
OctetString = univ.OctetString
Null = univ.Null
null = Null('')
ObjectIdentifier = univ.ObjectIdentifier

IpAddress = rfc1155.IpAddress
NetworkAddress = rfc1155.NetworkAddress
Counter = rfc1155.Counter
Gauge = rfc1155.Gauge
TimeTicks = rfc1155.TimeTicks
Opaque = rfc1155.Opaque

VarBind = rfc1157.VarBind
VarBindList = rfc1157.VarBindList
GetRequestPDU = rfc1157.GetRequestPDU
GetNextRequestPDU = rfc1157.GetNextRequestPDU
GetResponsePDU = rfc1157.GetResponsePDU
SetRequestPDU = rfc1157.SetRequestPDU
TrapPDU = rfc1157.TrapPDU
Message = rfc1157.Message


class VarBindAPI(object):
    @staticmethod
    def setOIDVal(varBind, oidVal):
        oid, val = oidVal[0], oidVal[1]
        varBind.setComponentByPosition(0, oid)
        if val is None:
            val = null
        varBind.setComponentByPosition(1).getComponentByPosition(1).setComponentByType(val.getTagSet(), val, verifyConstraints=False, matchTags=False, matchConstraints=False, innerFlag=True)
        return varBind

    @staticmethod
    def getOIDVal(varBind):
        return varBind[0], varBind[1].getComponent(1)


apiVarBind = VarBindAPI()

getNextRequestID = nextid.Integer(0xffffff)


class PDUAPI(object):
    _errorStatus = rfc1157.errorStatus.clone(0)
    _errorIndex = Integer(0)

    def setDefaults(self, pdu):
        pdu.setComponentByPosition(
            0, getNextRequestID(), verifyConstraints=False, matchTags=False, matchConstraints=False
        )
        pdu.setComponentByPosition(
            1, self._errorStatus, verifyConstraints=False, matchTags=False, matchConstraints=False
        )
        pdu.setComponentByPosition(
            2, self._errorIndex, verifyConstraints=False, matchTags=False, matchConstraints=False
        )
        pdu.setComponentByPosition(3)

    @staticmethod
    def getRequestID(pdu):
        return pdu.getComponentByPosition(0)

    @staticmethod
    def setRequestID(pdu, value):
        pdu.setComponentByPosition(0, value)

    @staticmethod
    def getErrorStatus(pdu):
        return pdu.getComponentByPosition(1)

    @staticmethod
    def setErrorStatus(pdu, value):
        pdu.setComponentByPosition(1, value)

    @staticmethod
    def getErrorIndex(pdu, muteErrors=False):
        errorIndex = pdu.getComponentByPosition(2)
        if errorIndex > len(pdu[3]):
            if muteErrors:
                return errorIndex.clone(len(pdu[3]))
            raise error.ProtocolError(
                'Error index out of range: %s > %s' % (errorIndex, len(pdu[3]))
            )
        return errorIndex

    @staticmethod
    def setErrorIndex(pdu, value):
        pdu.setComponentByPosition(2, value)

    def setEndOfMibError(self, pdu, errorIndex):
        self.setErrorIndex(pdu, errorIndex)
        self.setErrorStatus(pdu, 2)

    def setNoSuchInstanceError(self, pdu, errorIndex):
        self.setEndOfMibError(pdu, errorIndex)

    @staticmethod
    def getVarBindList(pdu):
        return pdu.getComponentByPosition(3)

    @staticmethod
    def setVarBindList(pdu, varBindList):
        pdu.setComponentByPosition(3, varBindList)

    @staticmethod
    def getVarBinds(pdu):
        return [apiVarBind.getOIDVal(varBind) for varBind in pdu.getComponentByPosition(3)]

    @staticmethod
    def setVarBinds(pdu, varBinds):
        varBindList = pdu.setComponentByPosition(3).getComponentByPosition(3)
        varBindList.clear()
        idx = 0
        for varBind in varBinds:
            if isinstance(varBind, VarBind):
                varBindList.setComponentByPosition(idx, varBind)
            else:
                varBindList.setComponentByPosition(idx)
                apiVarBind.setOIDVal(
                    varBindList.getComponentByPosition(idx), varBind
                )
            idx += 1

    def getResponse(self, reqPDU):
        rspPDU = GetResponsePDU()
        self.setDefaults(rspPDU)
        self.setRequestID(rspPDU, self.getRequestID(reqPDU))
        return rspPDU

    def getVarBindTable(self, reqPDU, rspPDU):
        if apiPDU.getErrorStatus(rspPDU) == 2:
            varBindRow = []
            for varBind in apiPDU.getVarBinds(reqPDU):
                varBindRow.append((varBind[0], null))
            return [varBindRow]
        else:
            return [apiPDU.getVarBinds(rspPDU)]


apiPDU = PDUAPI()


class TrapPDUAPI(object):
    _networkAddress = None
    _entOid = ObjectIdentifier((1, 3, 6, 1, 4, 1, 20408))
    _genericTrap = rfc1157.genericTrap.clone('coldStart')
    _zeroInt = univ.Integer(0)
    _zeroTime = TimeTicks(0)

    def setDefaults(self, pdu):
        if self._networkAddress is None:
            try:
                import socket
                agentAddress = IpAddress(socket.gethostbyname(socket.gethostname()))
            except Exception:
                agentAddress = IpAddress('0.0.0.0')
            self._networkAddress = NetworkAddress().setComponentByPosition(0, agentAddress)
        pdu.setComponentByPosition(0, self._entOid, verifyConstraints=False, matchTags=False, matchConstraints=False)
        pdu.setComponentByPosition(1, self._networkAddress, verifyConstraints=False, matchTags=False, matchConstraints=False)
        pdu.setComponentByPosition(2, self._genericTrap, verifyConstraints=False, matchTags=False, matchConstraints=False)
        pdu.setComponentByPosition(3, self._zeroInt, verifyConstraints=False, matchTags=False, matchConstraints=False)
        pdu.setComponentByPosition(4, self._zeroTime, verifyConstraints=False, matchTags=False, matchConstraints=False)
        pdu.setComponentByPosition(5)

    @staticmethod
    def getEnterprise(pdu):
        return pdu.getComponentByPosition(0)

    @staticmethod
    def setEnterprise(pdu, value):
        pdu.setComponentByPosition(0, value)

    @staticmethod
    def getAgentAddr(pdu):
        return pdu.getComponentByPosition(1).getComponentByPosition(0)

    @staticmethod
    def setAgentAddr(pdu, value):
        pdu.setComponentByPosition(1).getComponentByPosition(1).setComponentByPosition(0, value)

    @staticmethod
    def getGenericTrap(pdu):
        return pdu.getComponentByPosition(2)

    @staticmethod
    def setGenericTrap(pdu, value):
        pdu.setComponentByPosition(2, value)

    @staticmethod
    def getSpecificTrap(pdu):
        return pdu.getComponentByPosition(3)

    @staticmethod
    def setSpecificTrap(pdu, value):
        pdu.setComponentByPosition(3, value)

    @staticmethod
    def getTimeStamp(pdu):
        return pdu.getComponentByPosition(4)

    @staticmethod
    def setTimeStamp(pdu, value):
        pdu.setComponentByPosition(4, value)

    @staticmethod
    def getVarBindList(pdu):
        return pdu.getComponentByPosition(5)

    @staticmethod
    def setVarBindList(pdu, varBindList):
        pdu.setComponentByPosition(5, varBindList)

    @staticmethod
    def getVarBinds(pdu):
        varBinds = []
        for varBind in pdu.getComponentByPosition(5):
            varBinds.append(apiVarBind.getOIDVal(varBind))
        return varBinds

    @staticmethod
    def setVarBinds(pdu, varBinds):
        varBindList = pdu.setComponentByPosition(5).getComponentByPosition(5)
        varBindList.clear()
        idx = 0
        for varBind in varBinds:
            if isinstance(varBind, VarBind):
                varBindList.setComponentByPosition(idx, varBind)
            else:
                varBindList.setComponentByPosition(idx)
                apiVarBind.setOIDVal(
                    varBindList.getComponentByPosition(idx), varBind
                )
            idx += 1


apiTrapPDU = TrapPDUAPI()


class MessageAPI(object):
    _version = rfc1157.version.clone(0)
    _community = univ.OctetString('public')

    def setDefaults(self, msg):
        msg.setComponentByPosition(0, self._version, verifyConstraints=False, matchTags=False, matchConstraints=False)
        msg.setComponentByPosition(1, self._community, verifyConstraints=False, matchTags=False, matchConstraints=False)
        return msg

    @staticmethod
    def getVersion(msg):
        return msg.getComponentByPosition(0)

    @staticmethod
    def setVersion(msg, value):
        msg.setComponentByPosition(0, value)

    @staticmethod
    def getCommunity(msg):
        return msg.getComponentByPosition(1)

    @staticmethod
    def setCommunity(msg, value):
        msg.setComponentByPosition(1, value)

    @staticmethod
    def getPDU(msg):
        return msg.getComponentByPosition(2).getComponent()

    @staticmethod
    def setPDU(msg, value):
        msg.setComponentByPosition(2).getComponentByPosition(2).setComponentByType(value.getTagSet(), value, verifyConstraints=False, matchTags=False, matchConstraints=False, innerFlag=True)

    def getResponse(self, reqMsg):
        rspMsg = Message()
        self.setDefaults(rspMsg)
        self.setVersion(rspMsg, self.getVersion(reqMsg))
        self.setCommunity(rspMsg, self.getCommunity(reqMsg))
        self.setPDU(rspMsg, apiPDU.getResponse(self.getPDU(reqMsg)))
        return rspMsg


apiMessage = MessageAPI()
