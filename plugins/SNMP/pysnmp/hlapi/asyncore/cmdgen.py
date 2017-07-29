#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.entity.rfc3413 import cmdgen
from pysnmp.smi.rfc1902 import *
from pysnmp.hlapi.auth import *
from pysnmp.hlapi.context import *
from pysnmp.hlapi.lcd import *
from pysnmp.hlapi.varbinds import *
from pysnmp.hlapi.asyncore.transport import *

__all__ = ['getCmd', 'nextCmd', 'setCmd', 'bulkCmd', 'isEndOfMib']

vbProcessor = CommandGeneratorVarBinds()
lcd = CommandGeneratorLcdConfigurator()

isEndOfMib = lambda x: not cmdgen.getNextVarBinds(x)[1]


def getCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Performs SNMP GET query.

    Based on passed parameters, prepares SNMP GET packet
    (:RFC:`1905#section-4.2.1`) and schedules its transmission by
    I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer
        address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName
        values.

    \*varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
            * `cbFun` (callable) - user-supplied callable that is invoked
               to pass SNMP response data or error to user at a later point
               of time. Default is `None`.
            * `cbCtx` (object) - user-supplied object passing additional
               parameters to/from `cbFun`. Default is `None`.

    Notes
    -----
    User-supplied `cbFun` callable must have the following call
    signature:

    * snmpEngine (:py:class:`~pysnmp.hlapi.SnmpEngine`):
      Class instance representing SNMP engine.
    * sendRequestHandle (int): Unique request identifier. Can be used
      for matching multiple ongoing requests with received responses.
    * errorIndication (str): True value indicates SNMP engine error.
    * errorStatus (str): True value indicates SNMP PDU error.
    * errorIndex (int): Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple): A sequence of
      :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
      representing MIB variables returned in SNMP response in exactly
      the same order as `varBinds` in request.
    * `cbCtx` : Original user-supplied object.

    Returns
    -------
    sendRequestHandle : int
        Unique request identifier. Can be used for matching received
        responses with ongoing requests.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Examples
    --------
    >>> from pysnmp.hlapi.asyncore import *
    >>> def cbFun(snmpEngine, sendRequestHandle, errorIndication, errorStatus, errorIndex, varBinds, cbCtx):
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> snmpEngine = SnmpEngine()
    >>> getCmd(snmpEngine,
    ...        CommunityData('public'),
    ...        UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...        ContextData(),
    ...        ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)),
    ...        cbFun=cbFun)
    >>> snmpEngine.transportDispatcher.runDispatcher()
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBinds, cbCtx):
        lookupMib, cbFun, cbCtx = cbCtx
        if cbFun:
            return cbFun(snmpEngine, sendRequestHandle, errorIndication,
                         errorStatus, errorIndex,
                         vbProcessor.unmakeVarBinds(
                             snmpEngine, varBinds, lookupMib
                         ), cbCtx)

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    return cmdgen.GetCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True),
         options.get('cbFun'), options.get('cbCtx'))
    )


def setCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Performs SNMP SET query.

    Based on passed parameters, prepares SNMP SET packet
    (:RFC:`1905#section-4.2.5`) and schedules its transmission by
    I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer
        address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName
        values.

    \*varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
            * `cbFun` (callable) - user-supplied callable that is invoked
               to pass SNMP response data or error to user at a later point
               of time. Default is `None`.
            * `cbCtx` (object) - user-supplied object passing additional
               parameters to/from `cbFun`. Default is `None`.

    Notes
    -----
    User-supplied `cbFun` callable must have the following call
    signature:

    * snmpEngine (:py:class:`~pysnmp.hlapi.SnmpEngine`):
      Class instance representing SNMP engine.
    * sendRequestHandle (int): Unique request identifier. Can be used
      for matching multiple ongoing requests with received responses.
    * errorIndication (str): True value indicates SNMP engine error.
    * errorStatus (str): True value indicates SNMP PDU error.
    * errorIndex (int): Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple): A sequence of
      :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
      representing MIB variables returned in SNMP response in exactly
      the same order as `varBinds` in request.
    * `cbCtx` : Original user-supplied object.

    Returns
    -------
    sendRequestHandle : int
        Unique request identifier. Can be used for matching received
        responses with ongoing requests.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Examples
    --------
    >>> from pysnmp.hlapi.asyncore import *
    >>> def cbFun(snmpEngine, sendRequestHandle, errorIndication, errorStatus, errorIndex, varBinds, cbCtx):
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> snmpEngine = SnmpEngine()
    >>> setCmd(snmpEngine,
    ...        CommunityData('public'),
    ...        UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...        ContextData(),
    ...        ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysContact', 0), 'info@snmplabs.com'),
    ...        cbFun=cbFun)
    >>> snmpEngine.transportDispatcher.runDispatcher()
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.4.0')), DisplayString('info@snmplabs.com'))])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBinds, cbCtx):
        lookupMib, cbFun, cbCtx = cbCtx
        return cbFun(snmpEngine, sendRequestHandle, errorIndication,
                     errorStatus, errorIndex,
                     vbProcessor.unmakeVarBinds(
                         snmpEngine, varBinds, lookupMib
                     ), cbCtx)

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    return cmdgen.SetCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName, vbProcessor.makeVarBinds(snmpEngine, varBinds),
        __cbFun, (options.get('lookupMib', True),
                  options.get('cbFun'), options.get('cbCtx'))
    )


def nextCmd(snmpEngine, authData, transportTarget, contextData,
            *varBinds, **options):
    """Performs SNMP GETNEXT query.

    Based on passed parameters, prepares SNMP GETNEXT packet
    (:RFC:`1905#section-4.2.2`) and schedules its transmission by
    I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer
        address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName
        values.

    \*varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
            * `cbFun` (callable) - user-supplied callable that is invoked
               to pass SNMP response data or error to user at a later point
               of time. Default is `None`.
            * `cbCtx` (object) - user-supplied object passing additional
               parameters to/from `cbFun`. Default is `None`.

    Notes
    -----
    User-supplied `cbFun` callable must have the following call
    signature:

    * snmpEngine (:py:class:`~pysnmp.hlapi.SnmpEngine`):
      Class instance representing SNMP engine.
    * sendRequestHandle (int): Unique request identifier. Can be used
      for matching multiple ongoing requests with received responses.
    * errorIndication (str): True value indicates SNMP engine error.
    * errorStatus (str): True value indicates SNMP PDU error.
    * errorIndex (int): Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple): A sequence of sequences (e.g. 2-D array) of
      :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
      representing a table of MIB variables returned in SNMP response.
      Inner sequences represent table rows and ordered exactly the same
      as `varBinds` in request. Response to GETNEXT always contain a
      single row.
    * `cbCtx` : Original user-supplied object.

    Returns
    -------
    sendRequestHandle : int
        Unique request identifier. Can be used for matching received
        responses with ongoing requests.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Examples
    --------
    >>> from pysnmp.hlapi.asyncore import *
    >>> def cbFun(snmpEngine, sendRequestHandle, errorIndication, errorStatus, errorIndex, varBinds, cbCtx):
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> snmpEngine = SnmpEngine()
    >>> nextCmd(snmpEngine,
    ...         CommunityData('public'),
    ...         UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...         ContextData(),
    ...         ObjectType(ObjectIdentity('SNMPv2-MIB', 'system')),
    ...         cbFun=cbFun)
    >>> snmpEngine.transportDispatcher.runDispatcher()
    (None, 0, 0, [ [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))] ])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle, errorIndication,
                errorStatus, errorIndex, varBindTable, cbCtx):
        lookupMib, cbFun, cbCtx = cbCtx
        return cbFun(snmpEngine, sendRequestHandle, errorIndication,
                     errorStatus, errorIndex,
                     [vbProcessor.unmakeVarBinds(snmpEngine, varBindTableRow, lookupMib) for varBindTableRow in
                      varBindTable],
                     cbCtx)

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)
    return cmdgen.NextCommandGenerator().sendVarBinds(
        snmpEngine, addrName,
        contextData.contextEngineId, contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds),
        __cbFun, (options.get('lookupMib', True),
                  options.get('cbFun'), options.get('cbCtx'))
    )


def bulkCmd(snmpEngine, authData, transportTarget, contextData,
            nonRepeaters, maxRepetitions, *varBinds, **options):
    """Performs SNMP GETBULK query.

    Based on passed parameters, prepares SNMP GETBULK packet
    (:RFC:`1905#section-4.2.3`) and schedules its transmission by
    I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncore.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncore.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer
        address.

    contextData : :py:class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName
        values.

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
            * `cbFun` (callable) - user-supplied callable that is invoked
               to pass SNMP response data or error to user at a later point
               of time. Default is `None`.
            * `cbCtx` (object) - user-supplied object passing additional
               parameters to/from `cbFun`. Default is `None`.

    Notes
    -----
    User-supplied `cbFun` callable must have the following call
    signature:

    * snmpEngine (:py:class:`~pysnmp.hlapi.SnmpEngine`):
      Class instance representing SNMP engine.
    * sendRequestHandle (int): Unique request identifier. Can be used
      for matching multiple ongoing requests with received responses.
    * errorIndication (str): True value indicates SNMP engine error.
    * errorStatus (str): True value indicates SNMP PDU error.
    * errorIndex (int): Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple): A sequence of sequences (e.g. 2-D array) of
      :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
      representing a table of MIB variables returned in SNMP response.
      Inner sequences represent table rows and ordered exactly the same
      as `varBinds` in request. Number of rows might be less or equal
      to `maxRepetitions` value in request.
    * `cbCtx` : Original user-supplied object.

    Returns
    -------
    sendRequestHandle : int
        Unique request identifier. Can be used for matching received
        responses with ongoing requests.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Examples
    --------
    >>> from pysnmp.hlapi.asyncore import *
    >>> def cbFun(snmpEngine, sendRequestHandle, errorIndication, errorStatus, errorIndex, varBinds, cbCtx):
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> snmpEngine = SnmpEngine()
    >>> bulkCmd(snmpEngine,
    ...         CommunityData('public'),
    ...         UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...         ContextData(),
    ...         0, 2,
    ...         ObjectType(ObjectIdentity('SNMPv2-MIB', 'system')),
    ...         cbFun=cbFun)
    >>> snmpEngine.transportDispatcher.runDispatcher()
    (None, 0, 0, [ [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m')), ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.2.0')), ObjectIdentifier('1.3.6.1.4.1.424242.1.1')] ])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBindTable, cbCtx):
        lookupMib, cbFun, cbCtx = cbCtx
        return cbFun(snmpEngine, sendRequestHandle, errorIndication,
                     errorStatus, errorIndex,
                     [vbProcessor.unmakeVarBinds(snmpEngine, varBindTableRow, lookupMib) for varBindTableRow in
                      varBindTable], cbCtx)

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)
    return cmdgen.BulkCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName, nonRepeaters, maxRepetitions,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True),
         options.get('cbFun'), options.get('cbCtx'))
    )
