#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.smi.rfc1902 import *
from pysnmp.hlapi.auth import *
from pysnmp.hlapi.context import *
from pysnmp.hlapi.lcd import *
from pysnmp.hlapi.varbinds import *
from pysnmp.hlapi.twisted.transport import *
from pysnmp.entity.rfc3413 import cmdgen
from pysnmp.proto import errind
from twisted.internet.defer import Deferred
from twisted.python.failure import Failure

__all__ = ['getCmd', 'nextCmd', 'setCmd', 'bulkCmd', 'isEndOfMib']

vbProcessor = CommandGeneratorVarBinds()
lcd = CommandGeneratorLcdConfigurator()

isEndOfMib = lambda x: not cmdgen.getNextVarBinds(x)[1]

def getCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Performs SNMP GET query.

    Based on passed parameters, prepares SNMP GET packet
    (:RFC:`1905#section-4.2.1`) and schedules its transmission by
    :mod:`twisted` I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :class:`~pysnmp.hlapi.CommunityData` or :class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :class:`~pysnmp.hlapi.twisted.UdpTransportTarget` or :class:`~pysnmp.hlapi.twisted.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    \*varBinds : :class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.

    Returns
    -------
    deferred : :class:`~twisted.internet.defer.Deferred`
        Twisted Deferred object representing work-in-progress. User
        is expected to attach his own `success` and `error` callback
        functions to the Deferred object though
        :meth:`~twisted.internet.defer.Deferred.addCallbacks` method.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    User `success` callback is called with the following tuple as
    its first argument:

    * errorStatus (str) : True value indicates SNMP PDU error.
    * errorIndex (int) : Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple) : A sequence of
      :class:`~pysnmp.smi.rfc1902.ObjectType` class instances representing
      MIB variables returned in SNMP response.

    User `error` callback is called with `errorIndication` object wrapped
    in :class:`~twisted.python.failure.Failure` object.

    Examples
    --------
    >>> from twisted.internet.task import react
    >>> from pysnmp.hlapi.twisted import *
    >>>
    >>> def success(args):
    ...     (errorStatus, errorIndex, varBinds) = args
    ...     print(errorStatus, errorIndex, varBind)
    ...
    >>> def failure(errorIndication):
    ...     print(errorIndication)
    ...
    >>> def run(reactor):
    ...     d = getCmd(SnmpEngine(),
    ...                CommunityData('public'),
    ...                UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...                ContextData(),
    ...                ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)))
    ...     d.addCallback(success).addErrback(failure)
    ...     return d
    ...
    >>> react(run)
    (0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))])
    >>>

    """
    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBinds, cbCtx):
        lookupMib, deferred = cbCtx
        if errorIndication:
            deferred.errback(Failure(errorIndication))
        else:
            deferred.callback(
                (errorStatus, errorIndex,
                 vbProcessor.unmakeVarBinds(snmpEngine, varBinds, lookupMib))
            )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    deferred = Deferred()

    cmdgen.GetCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True), deferred)
    )
    return deferred

def setCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Performs SNMP SET query.

    Based on passed parameters, prepares SNMP SET packet
    (:RFC:`1905#section-4.2.5`) and schedules its transmission by
    :mod:`twisted` I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :class:`~pysnmp.hlapi.CommunityData` or :class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :class:`~pysnmp.hlapi.twisted.UdpTransportTarget` or :class:`~pysnmp.hlapi.twisted.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    \*varBinds : :class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.

    Returns
    -------
    deferred : :class:`~twisted.internet.defer.Deferred`
        Twisted Deferred object representing work-in-progress. User
        is expected to attach his own `success` and `error` callback
        functions to the Deferred object though
        :meth:`~twisted.internet.defer.Deferred.addCallbacks` method.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    User `success` callback is called with the following tuple as
    its first argument:

    * errorStatus (str) : True value indicates SNMP PDU error.
    * errorIndex (int) : Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple) : A sequence of
      :class:`~pysnmp.smi.rfc1902.ObjectType` class instances representing
      MIB variables returned in SNMP response.

    User `error` callback is called with `errorIndication` object wrapped
    in :class:`~twisted.python.failure.Failure` object.

    Examples
    --------
    >>> from twisted.internet.task import react
    >>> from pysnmp.hlapi.twisted import *
    >>>
    >>> def success(args):
    ...     (errorStatus, errorIndex, varBinds) = args
    ...     print(errorStatus, errorIndex, varBind)
    ...
    >>> def failure(errorIndication):
    ...     print(errorIndication)
    ...
    >>> def run(reactor):
    ...     d = setCmd(SnmpEngine(),
    ...                CommunityData('public'),
    ...                UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...                ContextData(),
    ...                ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0), 'Linux i386')
    ...     d.addCallback(success).addErrback(failure)
    ...     return d
    ...
    >>> react(run)
    (0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('Linux i386'))])
    >>>

    """
    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBinds, cbCtx):
        lookupMib, deferred = cbCtx
        if errorIndication:
            deferred.errback(Failure(errorIndication))
        else:
            deferred.callback(
                (errorStatus, errorIndex,
                 vbProcessor.unmakeVarBinds(snmpEngine, varBinds, lookupMib))
            )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    deferred = Deferred()

    cmdgen.SetCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True), deferred)
    )
    return deferred

def nextCmd(snmpEngine, authData, transportTarget, contextData,
            *varBinds, **options):
    """Performs SNMP GETNEXT query.

    Based on passed parameters, prepares SNMP GETNEXT packet
    (:RFC:`1905#section-4.2.2`) and schedules its transmission by
    :mod:`twisted` I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :class:`~pysnmp.hlapi.CommunityData` or :class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :class:`~pysnmp.hlapi.twisted.UdpTransportTarget` or :class:`~pysnmp.hlapi.twisted.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    \*varBinds : :class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
            * `ignoreNonIncreasingOid` - continue iteration even if response
              MIB variables (OIDs) are not greater then request MIB variables.
              Be aware that setting it to `True` may cause infinite loop between
              SNMP management and agent applications. Default is `False`.

    Returns
    -------
    deferred : :class:`~twisted.internet.defer.Deferred`
        Twisted Deferred object representing work-in-progress. User
        is expected to attach his own `success` and `error` callback
        functions to the Deferred object though
        :meth:`~twisted.internet.defer.Deferred.addCallbacks` method.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    User `success` callback is called with the following tuple as
    its first argument:

    * errorStatus (str) : True value indicates SNMP PDU error.
    * errorIndex (int) : Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple) :
        A sequence of sequences (e.g. 2-D array) of
        :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
        representing a table of MIB variables returned in SNMP response.
        Inner sequences represent table rows and ordered exactly the same
        as `varBinds` in request. Response to GETNEXT always contain
        a single row.

    User `error` callback is called with `errorIndication` object wrapped
    in :class:`~twisted.python.failure.Failure` object.

    Examples
    --------
    >>> from twisted.internet.task import react
    >>> from pysnmp.hlapi.twisted import *
    >>>
    >>> def success(args):
    ...     (errorStatus, errorIndex, varBindTable) = args
    ...     print(errorStatus, errorIndex, varBindTable)
    ...
    >>> def failure(errorIndication):
    ...     print(errorIndication)
    ...
    >>> def run(reactor):
    ...     d = nextCmd(SnmpEngine(),
    ...                 CommunityData('public'),
    ...                 UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...                 ContextData(),
    ...                 ObjectType(ObjectIdentity('SNMPv2-MIB', 'system'))
    ...     d.addCallback(success).addErrback(failure)
    ...     return d
    ...
    >>> react(run)
    (0, 0, [[ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))]])
    >>>

    """
    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBindTable, cbCtx):
        lookupMib, deferred = cbCtx
        if options.get('ignoreNonIncreasingOid', False) and errorIndication and \
                isinstance(errorIndication, errind.OidNotIncreasing):
            errorIndication = None
        if errorIndication:
            deferred.errback(Failure(errorIndication))
        else:
            deferred.callback(
                (errorStatus, errorIndex,
                 [vbProcessor.unmakeVarBinds(snmpEngine, varBindTableRow, lookupMib) for varBindTableRow in varBindTable])
            )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    deferred = Deferred()

    cmdgen.NextCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True), deferred)
    )
    return deferred

def bulkCmd(snmpEngine, authData, transportTarget, contextData,
            nonRepeaters, maxRepetitions, *varBinds, **options):
    """Performs SNMP GETBULK query.

    Based on passed parameters, prepares SNMP GETNEXT packet
    (:RFC:`1905#section-4.2.3`) and schedules its transmission by
    :mod:`twisted` I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :class:`~pysnmp.hlapi.CommunityData` or :class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :class:`~pysnmp.hlapi.twisted.UdpTransportTarget` or :class:`~pysnmp.hlapi.twisted.Udp6TransportTarget`
        Class instance representing transport type along with SNMP peer address.

    contextData : :class:`~pysnmp.hlapi.ContextData`
        Class instance representing SNMP ContextEngineId and ContextName values.

    nonRepeaters : int
        One MIB variable is requested in response for the first
        `nonRepeaters` MIB variables in request.

    maxRepetitions : int
        `maxRepetitions` MIB variables are requested in response for each
        of the remaining MIB variables in the request (e.g. excluding
        `nonRepeaters`). Remote SNMP engine may choose lesser value than
        requested.

    \*varBinds : :class:`~pysnmp.smi.rfc1902.ObjectType`
        One or more class instances representing MIB variables to place
        into SNMP request.

    Other Parameters
    ----------------
    \*\*options :
        Request options:

            * `lookupMib` - load MIB and resolve response MIB variables at
              the cost of slightly reduced performance. Default is `True`.
            * `ignoreNonIncreasingOid` - continue iteration even if response
              MIB variables (OIDs) are not greater then request MIB variables.
              Be aware that setting it to `True` may cause infinite loop between
              SNMP management and agent applications. Default is `False`.

    Returns
    -------
    deferred : :class:`~twisted.internet.defer.Deferred`
        Twisted Deferred object representing work-in-progress. User
        is expected to attach his own `success` and `error` callback
        functions to the Deferred object though
        :meth:`~twisted.internet.defer.Deferred.addCallbacks` method.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Notes
    -----
    User `success` callback is called with the following tuple as
    its first argument:

    * errorStatus (str) : True value indicates SNMP PDU error.
    * errorIndex (int) : Non-zero value refers to `varBinds[errorIndex-1]`
    * varBinds (tuple) :
        A sequence of sequences (e.g. 2-D array) of
        :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
        representing a table of MIB variables returned in SNMP response.
        Inner sequences represent table rows and ordered exactly the same
        as `varBinds` in request. Number of rows might be less or equal
        to `maxRepetitions` value in request.

    User `error` callback is called with `errorIndication` object wrapped
    in :class:`~twisted.python.failure.Failure` object.

    Examples
    --------
    >>> from twisted.internet.task import react
    >>> from pysnmp.hlapi.twisted import *
    >>>
    >>> def success(args):
    ...     (errorStatus, errorIndex, varBindTable) = args
    ...     print(errorStatus, errorIndex, varBindTable)
    ...
    >>> def failure(errorIndication):
    ...     print(errorIndication)
    ...
    >>> def run(reactor):
    ...     d = bulkCmd(SnmpEngine(),
    ...                 CommunityData('public'),
    ...                 UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...                 ContextData(),
    ...                 0, 2,
    ...                 ObjectType(ObjectIdentity('SNMPv2-MIB', 'system'))
    ...     d.addCallback(success).addErrback(failure)
    ...     return d
    ...
    >>> react(run)
    (0, 0, [[ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m')), ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.2.0')), ObjectIdentifier('1.3.6.1.4.1.424242.1.1')]])
    >>>

    """
    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBindTable, cbCtx):
        lookupMib, deferred = cbCtx
        if options.get('ignoreNonIncreasingOid', False) and errorIndication and \
                isinstance(errorIndication, errind.OidNotIncreasing):
            errorIndication = None
        if errorIndication:
            deferred.errback(Failure(errorIndication))
        else:
            deferred.callback(
                (errorStatus, errorIndex,
                 [vbProcessor.unmakeVarBinds(snmpEngine, varBindTableRow, lookupMib) for varBindTableRow in varBindTable])
            )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    deferred = Deferred()

    cmdgen.BulkCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName, nonRepeaters, maxRepetitions,
        vbProcessor.makeVarBinds(snmpEngine, varBinds),
        __cbFun,
        (options.get('lookupMib', True), deferred)
    )
    return deferred
