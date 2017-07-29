#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from sys import version_info
from pysnmp.hlapi.asyncore import cmdgen
from pysnmp.hlapi.varbinds import *
from pysnmp.proto.rfc1905 import endOfMibView
from pysnmp.proto import errind
from pyasn1.type.univ import Null

__all__ = ['getCmd', 'nextCmd', 'setCmd', 'bulkCmd']

if version_info[:2] < (2, 6):
    __all__.append('next')

    # noinspection PyShadowingBuiltins
    def next(iter):
        return iter.next()


def getCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Creates a generator to perform one or more SNMP GET queries.

    On each iteration, new SNMP GET request is send (:RFC:`1905#section-4.2.1`).
    The iterator blocks waiting for response to arrive or error to occur.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    \*varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.

    Yields
    ------
    errorIndication : str
        True value indicates SNMP engine error.
    errorStatus : str
        True value indicates SNMP PDU error.
    errorIndex : int
        Non-zero value refers to `varBinds[errorIndex-1]`
    varBinds : tuple
        A sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType` class
        instances representing MIB variables returned in SNMP response.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    The `getCmd` generator will be exhausted immidiately unless
    a new sequence of `varBinds` are send back into running generator
    (supported since Python 2.6).

    Examples
    --------
    >>> from pysnmp.hlapi import *
    >>> g = getCmd(SnmpEngine(),
    ...            CommunityData('public'),
    ...            UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...            ContextData(),
    ...            ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)))
    >>> next(g)
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))])
    >>>

    """

    # noinspection PyShadowingNames
    def cbFun(snmpEngine, sendRequestHandle,
              errorIndication, errorStatus, errorIndex,
              varBinds, cbCtx):
        cbCtx['errorIndication'] = errorIndication
        cbCtx['errorStatus'] = errorStatus
        cbCtx['errorIndex'] = errorIndex
        cbCtx['varBinds'] = varBinds

    cbCtx = {}

    while True:
        if varBinds:
            cmdgen.getCmd(snmpEngine, authData, transportTarget,
                          contextData, *varBinds,
                          **dict(cbFun=cbFun, cbCtx=cbCtx,
                                 lookupMib=options.get('lookupMib', True)))

            snmpEngine.transportDispatcher.runDispatcher()

            errorIndication = cbCtx['errorIndication']
            errorStatus = cbCtx['errorStatus']
            errorIndex = cbCtx['errorIndex']
            varBinds = cbCtx['varBinds']
        else:
            errorIndication = errorStatus = errorIndex = None
            varBinds = []

        varBinds = (yield errorIndication, errorStatus, errorIndex, varBinds)

        if not varBinds:
            break


def setCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Creates a generator to perform one or more SNMP SET queries.

    On each iteration, new SNMP SET request is send (:RFC:`1905#section-4.2.5`).
    The iterator blocks waiting for response to arrive or error to occur.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    \*varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
              Default is `True`.

    Yields
    ------
    errorIndication : str
        True value indicates SNMP engine error.
    errorStatus : str
        True value indicates SNMP PDU error.
    errorIndex : int
        Non-zero value refers to `varBinds[errorIndex-1]`
    varBinds : tuple
        A sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType` class
        instances representing MIB variables returned in SNMP response.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    The `setCmd` generator will be exhausted immidiately unless
    a new sequence of `varBinds` are send back into running generator
    (supported since Python 2.6).

    Examples
    --------
    >>> from pysnmp.hlapi import *
    >>> g = setCmd(SnmpEngine(),
    ...            CommunityData('public'),
    ...            UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...            ContextData(),
    ...            ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0), 'Linux i386'))
    >>> next(g)
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('Linux i386'))])
    >>>

    """

    # noinspection PyShadowingNames
    def cbFun(snmpEngine, sendRequestHandle,
              errorIndication, errorStatus, errorIndex,
              varBinds, cbCtx):
        cbCtx['errorIndication'] = errorIndication
        cbCtx['errorStatus'] = errorStatus
        cbCtx['errorIndex'] = errorIndex
        cbCtx['varBinds'] = varBinds

    cbCtx = {}

    while True:
        if varBinds:
            cmdgen.setCmd(snmpEngine, authData, transportTarget,
                          contextData, *varBinds,
                          **dict(cbFun=cbFun, cbCtx=cbCtx,
                                 lookupMib=options.get('lookupMib', True)))

            snmpEngine.transportDispatcher.runDispatcher()

            errorIndication = cbCtx['errorIndication']
            errorStatus = cbCtx['errorStatus']
            errorIndex = cbCtx['errorIndex']
            varBinds = cbCtx['varBinds']
        else:
            errorIndication = errorStatus = errorIndex = None
            varBinds = []

        varBinds = (yield errorIndication, errorStatus, errorIndex, varBinds)

        if not varBinds:
            break


def nextCmd(snmpEngine, authData, transportTarget, contextData,
            *varBinds, **options):
    """Creates a generator to perform one or more SNMP GETNEXT queries.

    On each iteration, new SNMP GETNEXT request is send
    (:RFC:`1905#section-4.2.2`). The iterator blocks waiting for response
    to arrive or error to occur.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    \*varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
              Default is `True`.
            * `lexicographicMode` - walk SNMP agent's MIB till the end (if `True`),
              otherwise (if `False`) stop iteration when all response MIB
              variables leave the scope of initial MIB variables in
              `varBinds`. Default is `True`.
            * `ignoreNonIncreasingOid` - continue iteration even if response
              MIB variables (OIDs) are not greater then request MIB variables.
              Be aware that setting it to `True` may cause infinite loop between
              SNMP management and agent applications. Default is `False`.
            * `maxRows` - stop iteration once this generator instance processed
              `maxRows` of SNMP conceptual table. Default is `0` (no limit).
            * `maxCalls` - stop iteration once this generator instance processed
              `maxCalls` responses. Default is 0 (no limit).

    Yields
    ------
    errorIndication : str
        True value indicates SNMP engine error.
    errorStatus : str
        True value indicates SNMP PDU error.
    errorIndex : int
        Non-zero value refers to `varBinds[errorIndex-1]`
    varBinds : tuple
        A sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType` class
        instances representing MIB variables returned in SNMP response.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    The `nextCmd` generator will be exhausted on any of the following
    conditions:

    * SNMP engine error occurs thus `errorIndication` is `True`
    * SNMP PDU `errorStatus` is reported as `True`
    * SNMP :py:class:`~pysnmp.proto.rfc1905.EndOfMibView` values
      (also known as *SNMP exception values*) are reported for all
      MIB variables in `varBinds`
    * *lexicographicMode* option is `True` and SNMP agent reports
      end-of-mib or *lexicographicMode* is `False` and all
      response MIB variables leave the scope of `varBinds`

    At any moment a new sequence of `varBinds` could be send back into
    running generator (supported since Python 2.6).

    Examples
    --------
    >>> from pysnmp.hlapi import *
    >>> g = nextCmd(SnmpEngine(),
    ...             CommunityData('public'),
    ...             UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...             ContextData(),
    ...             ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr')))
    >>> next(g)
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))])
    >>> g.send( [ ObjectType(ObjectIdentity('IF-MIB', 'ifInOctets')) ] )
    (None, 0, 0, [(ObjectName('1.3.6.1.2.1.2.2.1.10.1'), Counter32(284817787))])
    """

    # noinspection PyShadowingNames
    def cbFun(snmpEngine, sendRequestHandle,
              errorIndication, errorStatus, errorIndex,
              varBindTable, cbCtx):
        cbCtx['errorIndication'] = errorIndication
        cbCtx['errorStatus'] = errorStatus
        cbCtx['errorIndex'] = errorIndex
        cbCtx['varBindTable'] = varBindTable

    lexicographicMode = options.get('lexicographicMode', True)
    ignoreNonIncreasingOid = options.get('ignoreNonIncreasingOid', False)
    maxRows = options.get('maxRows', 0)
    maxCalls = options.get('maxCalls', 0)

    cbCtx = {}

    vbProcessor = CommandGeneratorVarBinds()

    initialVars = [x[0] for x in vbProcessor.makeVarBinds(snmpEngine, varBinds)]

    totalRows = totalCalls = 0

    while True:
        if varBinds:
            cmdgen.nextCmd(snmpEngine, authData, transportTarget, contextData,
                           *[(x[0], Null()) for x in varBinds],
                           **dict(cbFun=cbFun, cbCtx=cbCtx,
                                  lookupMib=options.get('lookupMib', True)))

            snmpEngine.transportDispatcher.runDispatcher()

            errorIndication = cbCtx['errorIndication']
            errorStatus = cbCtx['errorStatus']
            errorIndex = cbCtx['errorIndex']

            if ignoreNonIncreasingOid and errorIndication and isinstance(errorIndication, errind.OidNotIncreasing):
                errorIndication = None

            if errorIndication:
                yield (errorIndication, errorStatus, errorIndex, varBinds)
                return
            elif errorStatus:
                if errorStatus == 2:
                    # Hide SNMPv1 noSuchName error which leaks in here
                    # from SNMPv1 Agent through internal pysnmp proxy.
                    errorStatus = errorStatus.clone(0)
                    errorIndex = errorIndex.clone(0)
                yield (errorIndication, errorStatus, errorIndex, varBinds)
                return
            else:
                varBinds = cbCtx['varBindTable'] and cbCtx['varBindTable'][0]
                for idx, varBind in enumerate(varBinds):
                    name, val = varBind
                    if not isinstance(val, Null):
                        if lexicographicMode or initialVars[idx].isPrefixOf(name):
                            break
                else:
                    return

                totalRows += 1
                totalCalls += 1
        else:
            errorIndication = errorStatus = errorIndex = None
            varBinds = []

        initialVarBinds = (yield errorIndication, errorStatus, errorIndex, varBinds)

        if initialVarBinds:
            varBinds = initialVarBinds
            initialVars = [x[0] for x in vbProcessor.makeVarBinds(snmpEngine, varBinds)]

        if maxRows and totalRows >= maxRows:
            return

        if maxCalls and totalCalls >= maxCalls:
            return


def bulkCmd(snmpEngine, authData, transportTarget, contextData,
            nonRepeaters, maxRepetitions, *varBinds, **options):
    """Creates a generator to perform one or more SNMP GETBULK queries.

    On each iteration, new SNMP GETBULK request is send
    (:RFC:`1905#section-4.2.3`). The iterator blocks waiting for response
    to arrive or error to occur.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    nonRepeaters : int
        One MIB variable is requested in response for the first
        `nonRepeaters` MIB variables in request.

    maxRepetitions : int
        `maxRepetitions` MIB variables are requested in response for each
        of the remaining MIB variables in the request (e.g. excluding
        `nonRepeaters`). Remote SNMP engine may choose lesser value than
        requested.

    \*varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
              Default is `True`.
            * `lexicographicMode` - walk SNMP agent's MIB till the end (if `True`),
              otherwise (if `False`) stop iteration when all response MIB
              variables leave the scope of initial MIB variables in
              `varBinds`. Default is `True`.
            * `ignoreNonIncreasingOid` - continue iteration even if response
              MIB variables (OIDs) are not greater then request MIB variables.
              Be aware that setting it to `True` may cause infinite loop between
              SNMP management and agent applications. Default is `False`.
            * `maxRows` - stop iteration once this generator instance processed
              `maxRows` of SNMP conceptual table. Default is `0` (no limit).
            * `maxCalls` - stop iteration once this generator instance processed
              `maxCalls` responses. Default is 0 (no limit).

    Yields
    ------
    errorIndication : str
        True value indicates SNMP engine error.
    errorStatus : str
        True value indicates SNMP PDU error.
    errorIndex : int
        Non-zero value refers to \*varBinds[errorIndex-1]
    varBinds : tuple
        A sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType` class
        instances representing MIB variables returned in SNMP response.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    The `bulkCmd` generator will be exhausted on any of the following
    conditions:

    * SNMP engine error occurs thus `errorIndication` is `True`
    * SNMP PDU `errorStatus` is reported as `True`
    * SNMP :py:class:`~pysnmp.proto.rfc1905.EndOfMibView` values
      (also known as *SNMP exception values*) are reported for all
      MIB variables in `varBinds`
    * *lexicographicMode* option is `True` and SNMP agent reports
      end-of-mib or *lexicographicMode* is `False` and all
      response MIB variables leave the scope of `varBinds`

    At any moment a new sequence of `varBinds` could be send back into
    running generator (supported since Python 2.6).

    Setting `maxRepetitions` value to 15..50 might significantly improve
    system performance, as many MIB variables get packed into a single
    response message at once.

    Examples
    --------
    >>> from pysnmp.hlapi import *
    >>> g = bulkCmd(SnmpEngine(),
    ...             CommunityData('public'),
    ...             UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...             ContextData(),
    ...             0, 25,
    ...             ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr')))
    >>> next(g)
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))])
    >>> g.send( [ ObjectType(ObjectIdentity('IF-MIB', 'ifInOctets')) ] )
    (None, 0, 0, [(ObjectName('1.3.6.1.2.1.2.2.1.10.1'), Counter32(284817787))])
    """

    # noinspection PyShadowingNames
    def cbFun(snmpEngine, sendRequestHandle,
              errorIndication, errorStatus, errorIndex,
              varBindTable, cbCtx):
        cbCtx['errorIndication'] = errorIndication
        cbCtx['errorStatus'] = errorStatus
        cbCtx['errorIndex'] = errorIndex
        cbCtx['varBindTable'] = varBindTable

    lexicographicMode = options.get('lexicographicMode', True)
    ignoreNonIncreasingOid = options.get('ignoreNonIncreasingOid', False)
    maxRows = options.get('maxRows', 0)
    maxCalls = options.get('maxCalls', 0)

    cbCtx = {}

    vbProcessor = CommandGeneratorVarBinds()

    initialVars = [x[0] for x in vbProcessor.makeVarBinds(snmpEngine, varBinds)]
    nullVarBinds = [False] * len(initialVars)

    totalRows = totalCalls = 0
    stopFlag = False

    while not stopFlag:
        if maxRows and totalRows < maxRows:
            maxRepetitions = min(maxRepetitions, maxRows - totalRows)

        cmdgen.bulkCmd(snmpEngine, authData, transportTarget, contextData,
                       nonRepeaters, maxRepetitions,
                       *[(x[0], Null()) for x in varBinds],
                       **dict(cbFun=cbFun, cbCtx=cbCtx,
                              lookupMib=options.get('lookupMib', True)))

        snmpEngine.transportDispatcher.runDispatcher()

        errorIndication = cbCtx['errorIndication']
        errorStatus = cbCtx['errorStatus']
        errorIndex = cbCtx['errorIndex']
        varBindTable = cbCtx['varBindTable']

        if ignoreNonIncreasingOid and errorIndication and \
                isinstance(errorIndication, errind.OidNotIncreasing):
            errorIndication = None

        if errorIndication:
            yield (errorIndication, errorStatus, errorIndex,
                   varBindTable and varBindTable[0] or [])
            if errorIndication != errind.requestTimedOut:
                return
        elif errorStatus:
            if errorStatus == 2:
                # Hide SNMPv1 noSuchName error which leaks in here
                # from SNMPv1 Agent through internal pysnmp proxy.
                errorStatus = errorStatus.clone(0)
                errorIndex = errorIndex.clone(0)
            yield (errorIndication, errorStatus, errorIndex, varBindTable and varBindTable[0] or [])
            return
        else:
            for row in range(len(varBindTable)):
                stopFlag = True
                if len(varBindTable[row]) != len(initialVars):
                    varBindTable = row and varBindTable[:row - 1] or []
                    break
                for col in range(len(varBindTable[row])):
                    name, val = varBindTable[row][col]
                    if nullVarBinds[col]:
                        varBindTable[row][col] = name, endOfMibView
                        continue
                    stopFlag = False
                    if isinstance(val, Null):
                        nullVarBinds[col] = True
                    elif not lexicographicMode and not initialVars[col].isPrefixOf(name):
                        varBindTable[row][col] = name, endOfMibView
                        nullVarBinds[col] = True
                if stopFlag:
                    varBindTable = row and varBindTable[:row - 1] or []
                    break

            totalRows += len(varBindTable)
            totalCalls += 1

            if maxRows and totalRows >= maxRows:
                if totalRows > maxRows:
                    varBindTable = varBindTable[:-(totalRows - maxRows)]
                stopFlag = True

            if maxCalls and totalCalls >= maxCalls:
                stopFlag = True

            for varBinds in varBindTable:
                initialVarBinds = (yield errorIndication, errorStatus, errorIndex, varBinds)

                if initialVarBinds:
                    varBinds = initialVarBinds
                    initialVars = [x[0] for x in vbProcessor.makeVarBinds(snmpEngine, varBinds)]
