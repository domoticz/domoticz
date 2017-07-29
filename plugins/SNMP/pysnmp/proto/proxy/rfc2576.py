#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto import rfc1905, rfc3411, error
from pysnmp.proto.api import v1, v2c
from pysnmp import debug

# 2.1.1

__v1ToV2ValueMap = {
    v1.Integer.tagSet: v2c.Integer32(),
    v1.OctetString.tagSet: v2c.OctetString(),
    v1.Null.tagSet: v2c.Null(),
    v1.ObjectIdentifier.tagSet: v2c.ObjectIdentifier(),
    v1.IpAddress.tagSet: v2c.IpAddress(),
    v1.Counter.tagSet: v2c.Counter32(),
    v1.Gauge.tagSet: v2c.Gauge32(),
    v1.TimeTicks.tagSet: v2c.TimeTicks(),
    v1.Opaque.tagSet: v2c.Opaque()
}

__v2ToV1ValueMap = {  # XXX do not re-create same-type items?
    v2c.Integer32.tagSet: v1.Integer(),
    v2c.OctetString.tagSet: v1.OctetString(),
    v2c.Null.tagSet: v1.Null(),
    v2c.ObjectIdentifier.tagSet: v1.ObjectIdentifier(),
    v2c.IpAddress.tagSet: v1.IpAddress(),
    v2c.Counter32.tagSet: v1.Counter(),
    v2c.Gauge32.tagSet: v1.Gauge(),
    v2c.TimeTicks.tagSet: v1.TimeTicks(),
    v2c.Opaque.tagSet: v1.Opaque()
}

# PDU map

__v1ToV2PduMap = {
    v1.GetRequestPDU.tagSet: v2c.GetRequestPDU(),
    v1.GetNextRequestPDU.tagSet: v2c.GetNextRequestPDU(),
    v1.SetRequestPDU.tagSet: v2c.SetRequestPDU(),
    v1.GetResponsePDU.tagSet: v2c.ResponsePDU(),
    v1.TrapPDU.tagSet: v2c.SNMPv2TrapPDU()
}

__v2ToV1PduMap = {
    v2c.GetRequestPDU.tagSet: v1.GetRequestPDU(),
    v2c.GetNextRequestPDU.tagSet: v1.GetNextRequestPDU(),
    v2c.SetRequestPDU.tagSet: v1.SetRequestPDU(),
    v2c.ResponsePDU.tagSet: v1.GetResponsePDU(),
    v2c.SNMPv2TrapPDU.tagSet: v1.TrapPDU(),
    v2c.GetBulkRequestPDU.tagSet: v1.GetNextRequestPDU()  # 4.1.1
}

# Trap map

__v1ToV2TrapMap = {
    0: (1, 3, 6, 1, 6, 3, 1, 1, 5, 1),
    1: (1, 3, 6, 1, 6, 3, 1, 1, 5, 2),
    2: (1, 3, 6, 1, 6, 3, 1, 1, 5, 3),
    3: (1, 3, 6, 1, 6, 3, 1, 1, 5, 4),
    4: (1, 3, 6, 1, 6, 3, 1, 1, 5, 5),
    5: (1, 3, 6, 1, 6, 3, 1, 1, 5, 6)
}

__v2ToV1TrapMap = {
    (1, 3, 6, 1, 6, 3, 1, 1, 5, 1): 0,
    (1, 3, 6, 1, 6, 3, 1, 1, 5, 2): 1,
    (1, 3, 6, 1, 6, 3, 1, 1, 5, 3): 2,
    (1, 3, 6, 1, 6, 3, 1, 1, 5, 4): 3,
    (1, 3, 6, 1, 6, 3, 1, 1, 5, 5): 4,
    (1, 3, 6, 1, 6, 3, 1, 1, 5, 6): 5
}

# 4.3

__v2ToV1ErrorMap = {
    0: 0,
    1: 1,
    5: 5,
    10: 3,
    9: 3,
    7: 3,
    8: 3,
    12: 3,
    6: 2,
    17: 2,
    11: 2,
    18: 2,
    13: 5,
    14: 5,
    15: 5,
    16: 2
}

__zeroInt = v1.Integer(0)


def v1ToV2(v1Pdu, origV2Pdu=None):
    pduType = v1Pdu.tagSet
    v2Pdu = __v1ToV2PduMap[pduType].clone()

    debug.logger & debug.flagPrx and debug.logger('v1ToV2: v1Pdu %s' % v1Pdu.prettyPrint())

    v2VarBinds = []

    # 3.1
    if pduType in rfc3411.notificationClassPDUs:
        # 3.1.1
        sysUpTime = v1.apiTrapPDU.getTimeStamp(v1Pdu)

        # 3.1.2
        genericTrap = v1.apiTrapPDU.getGenericTrap(v1Pdu)
        if genericTrap == 6:
            snmpTrapOIDParam = v1.apiTrapPDU.getEnterprise(v1Pdu) + (0, int(v1.apiTrapPDU.getSpecificTrap(v1Pdu)))

        # 3.1.3
        else:
            snmpTrapOIDParam = v2c.ObjectIdentifier(__v1ToV2TrapMap[genericTrap])

        # 3.1.4 (XXX snmpTrapCommunity.0 is missing here)
        v2VarBinds.append((v2c.apiTrapPDU.sysUpTime, sysUpTime))
        v2VarBinds.append((v2c.apiTrapPDU.snmpTrapOID, snmpTrapOIDParam))
        v2VarBinds.append(
            (v2c.apiTrapPDU.snmpTrapAddress, v1.apiTrapPDU.getAgentAddr(v1Pdu))
        )
        v2VarBinds.append((v2c.apiTrapPDU.snmpTrapCommunity, v2c.OctetString("")))
        v2VarBinds.append((v2c.apiTrapPDU.snmpTrapEnterprise,
                           v1.apiTrapPDU.getEnterprise(v1Pdu)))

        varBinds = v1.apiTrapPDU.getVarBinds(v1Pdu)
    else:
        varBinds = v1.apiPDU.getVarBinds(v1Pdu)

    # Translate Var-Binds
    for oid, v1Val in varBinds:
        # 2.1.1.11
        if v1Val.tagSet == v1.NetworkAddress.tagSet:
            v1Val = v1Val.getComponent()
        v2VarBinds.append(
            (oid, __v1ToV2ValueMap[v1Val.tagSet].clone(v1Val))
        )

    if pduType in rfc3411.responseClassPDUs:
        # 4.1.2.2.1&2
        errorStatus = int(v1.apiPDU.getErrorStatus(v1Pdu))
        errorIndex = int(v1.apiPDU.getErrorIndex(v1Pdu, muteErrors=True))
        if errorStatus == 2:  # noSuchName
            if origV2Pdu.tagSet == v2c.GetNextRequestPDU.tagSet:
                v2VarBinds = [(o, rfc1905.endOfMibView) for o, v in v2VarBinds]
            else:
                v2VarBinds = [(o, rfc1905.noSuchObject) for o, v in v2VarBinds]
            v2c.apiPDU.setErrorStatus(v2Pdu, 0)
            v2c.apiPDU.setErrorIndex(v2Pdu, 0)
        else:
            # partial one-to-one mapping - 4.2.1
            v2c.apiPDU.setErrorStatus(v2Pdu, errorStatus)
            v2c.apiPDU.setErrorIndex(v2Pdu, errorIndex)

            # 4.1.2.1 --> no-op

    elif pduType in rfc3411.confirmedClassPDUs:
        v2c.apiPDU.setErrorStatus(v2Pdu, 0)
        v2c.apiPDU.setErrorIndex(v2Pdu, 0)

    if pduType not in rfc3411.notificationClassPDUs:
        v2c.apiPDU.setRequestID(v2Pdu, int(v1.apiPDU.getRequestID(v1Pdu)))

    v2c.apiPDU.setVarBinds(v2Pdu, v2VarBinds)

    debug.logger & debug.flagPrx and debug.logger('v1ToV2: v2Pdu %s' % v2Pdu.prettyPrint())

    return v2Pdu


def v2ToV1(v2Pdu, origV1Pdu=None):
    debug.logger & debug.flagPrx and debug.logger('v2ToV1: v2Pdu %s' % v2Pdu.prettyPrint())

    pduType = v2Pdu.tagSet

    if pduType in __v2ToV1PduMap:
        v1Pdu = __v2ToV1PduMap[pduType].clone()
    else:
        raise error.ProtocolError('Unsupported PDU type')

    v2VarBinds = v2c.apiPDU.getVarBinds(v2Pdu)
    v1VarBinds = []

    # 3.2
    if pduType in rfc3411.notificationClassPDUs:
        # 3.2.1
        snmpTrapOID, snmpTrapOIDParam = v2VarBinds[1]
        if snmpTrapOID != v2c.apiTrapPDU.snmpTrapOID:
            raise error.ProtocolError('Second OID not snmpTrapOID')
        snmpTrapOID, snmpTrapOIDParam = v2VarBinds[1]
        if snmpTrapOIDParam in __v2ToV1TrapMap:
            for oid, val in v2VarBinds:
                if oid == v2c.apiTrapPDU.snmpTrapEnterprise:
                    v1.apiTrapPDU.setEnterprise(v1Pdu, val)
                    break
            else:
                # snmpTraps
                v1.apiTrapPDU.setEnterprise(v1Pdu, (1, 3, 6, 1, 6, 3, 1, 1, 5))
        else:
            if snmpTrapOIDParam[-2] == 0:
                v1.apiTrapPDU.setEnterprise(v1Pdu, snmpTrapOIDParam[:-2])
            else:
                v1.apiTrapPDU.setEnterprise(v1Pdu, snmpTrapOIDParam[:-1])

        # 3.2.2
        for oid, val in v2VarBinds:
            # snmpTrapAddress
            if oid == v2c.apiTrapPDU.snmpTrapAddress:
                v1.apiTrapPDU.setAgentAddr(v1Pdu, v1.IpAddress(val))  # v2c.OctetString is more constrained
                break
        else:
            v1.apiTrapPDU.setAgentAddr(v1Pdu, v1.IpAddress('0.0.0.0'))

        # 3.2.3
        if snmpTrapOIDParam in __v2ToV1TrapMap:
            v1.apiTrapPDU.setGenericTrap(v1Pdu, __v2ToV1TrapMap[snmpTrapOIDParam])
        else:
            v1.apiTrapPDU.setGenericTrap(v1Pdu, 6)

        # 3.2.4
        if snmpTrapOIDParam in __v2ToV1TrapMap:
            v1.apiTrapPDU.setSpecificTrap(v1Pdu, __zeroInt)
        else:
            v1.apiTrapPDU.setSpecificTrap(v1Pdu, snmpTrapOIDParam[-1])

        # 3.2.5
        v1.apiTrapPDU.setTimeStamp(v1Pdu, v2VarBinds[0][1])

        __v2VarBinds = []
        for oid, val in v2VarBinds[2:]:
            if oid in __v2ToV1TrapMap or \
                    oid in (v2c.apiTrapPDU.sysUpTime,
                            v2c.apiTrapPDU.snmpTrapAddress,
                            v2c.apiTrapPDU.snmpTrapEnterprise):
                continue
            __v2VarBinds.append((oid, val))
        v2VarBinds = __v2VarBinds

        # 3.2.6 --> done below

    else:
        v1.apiPDU.setErrorStatus(v1Pdu, __zeroInt)
        v1.apiPDU.setErrorIndex(v1Pdu, __zeroInt)

    if pduType in rfc3411.responseClassPDUs:
        idx = len(v2VarBinds) - 1
        while idx >= 0:
            # 4.1.2.1
            oid, val = v2VarBinds[idx]
            if v2c.Counter64.tagSet == val.tagSet:
                if origV1Pdu.tagSet == v1.GetRequestPDU.tagSet:
                    v1.apiPDU.setErrorStatus(v1Pdu, 2)
                    v1.apiPDU.setErrorIndex(v1Pdu, idx + 1)
                    break
                elif origV1Pdu.tagSet == v1.GetNextRequestPDU.tagSet:
                    raise error.StatusInformation(idx=idx, pdu=v2Pdu)
                else:
                    raise error.ProtocolError('Counter64 on the way')

            # 4.1.2.2.1&2
            if val.tagSet in (v2c.NoSuchObject.tagSet,
                              v2c.NoSuchInstance.tagSet,
                              v2c.EndOfMibView.tagSet):
                v1.apiPDU.setErrorStatus(v1Pdu, 2)
                v1.apiPDU.setErrorIndex(v1Pdu, idx + 1)

            idx -= 1

        # 4.1.2.3.1
        v2ErrorStatus = v2c.apiPDU.getErrorStatus(v2Pdu)
        if v2ErrorStatus:
            v1.apiPDU.setErrorStatus(
                v1Pdu, __v2ToV1ErrorMap.get(v2ErrorStatus, 5)
            )
            v1.apiPDU.setErrorIndex(v1Pdu, v2c.apiPDU.getErrorIndex(v2Pdu, muteErrors=True))

    elif pduType in rfc3411.confirmedClassPDUs:
        v1.apiPDU.setErrorStatus(v1Pdu, 0)
        v1.apiPDU.setErrorIndex(v1Pdu, 0)

    # Translate Var-Binds
    if pduType in rfc3411.responseClassPDUs and \
            v1.apiPDU.getErrorStatus(v1Pdu):
        v1VarBinds = v1.apiPDU.getVarBinds(origV1Pdu)
    else:
        for oid, v2Val in v2VarBinds:
            v1VarBinds.append(
                (oid, __v2ToV1ValueMap[v2Val.tagSet].clone(v2Val))
            )

    if pduType in rfc3411.notificationClassPDUs:
        v1.apiTrapPDU.setVarBinds(v1Pdu, v1VarBinds)
    else:
        v1.apiPDU.setVarBinds(v1Pdu, v1VarBinds)

        v1.apiPDU.setRequestID(
            v1Pdu, v2c.apiPDU.getRequestID(v2Pdu)
        )

    debug.logger & debug.flagPrx and debug.logger('v2ToV1: v1Pdu %s' % v1Pdu.prettyPrint())

    return v1Pdu
