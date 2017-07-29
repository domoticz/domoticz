#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# Copyright (C) 2014, Zebra Technologies
# Authors: Matt Hooks <me@matthooks.com>
#          Zachary Lorusso <zlorusso@gmail.com>
# Modified by Ilya Etingof <ilya@snmplabs.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
from pysnmp.smi.rfc1902 import *
from pysnmp.hlapi.auth import *
from pysnmp.hlapi.context import *
from pysnmp.hlapi.lcd import *
from pysnmp.hlapi.varbinds import *
from pysnmp.hlapi.asyncio.transport import *
from pysnmp.entity.rfc3413 import cmdgen

try:
    import asyncio
except ImportError:
    import trollius as asyncio

__all__ = ['getCmd', 'nextCmd', 'setCmd', 'bulkCmd', 'isEndOfMib']

vbProcessor = CommandGeneratorVarBinds()
lcd = CommandGeneratorLcdConfigurator()

isEndOfMib = lambda x: not cmdgen.getNextVarBinds(x)[1]


@asyncio.coroutine
def getCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Creates a generator to perform SNMP GET query.

    When itereator gets advanced by :py:mod:`asyncio` main loop,
    SNMP GET request is send (:RFC:`1905#section-4.2.1`).
    The iterator yields :py:class:`asyncio.Future` which gets done whenever
    response arrives or error occurs.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncio.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncio.Udp6TransportTarget`
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

    Examples
    --------
    >>> import asyncio
    >>> from pysnmp.hlapi.asyncio import *
    >>>
    >>> @asyncio.coroutine
    ... def run():
    ...     errorIndication, errorStatus, errorIndex, varBinds = yield from getCmd(
    ...         SnmpEngine(),
    ...         CommunityData('public'),
    ...         UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...         ContextData(),
    ...         ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0))
    ...     )
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> asyncio.get_event_loop().run_until_complete(run())
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m'))])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBinds, cbCtx):
        lookupMib, future = cbCtx
        if future.cancelled():
            return
        future.set_result(
            (errorIndication, errorStatus, errorIndex, vbProcessor.unmakeVarBinds(snmpEngine, varBinds, lookupMib))
        )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    future = asyncio.Future()

    cmdgen.GetCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True), future)
    )
    return future


@asyncio.coroutine
def setCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    """Creates a generator to perform SNMP SET query.

    When itereator gets advanced by :py:mod:`asyncio` main loop,
    SNMP SET request is send (:RFC:`1905#section-4.2.5`).
    The iterator yields :py:class:`asyncio.Future` which gets done whenever
    response arrives or error occurs.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncio.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncio.Udp6TransportTarget`
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

    Examples
    --------
    >>> import asyncio
    >>> from pysnmp.hlapi.asyncio import *
    >>>
    >>> @asyncio.coroutine
    ... def run():
    ...     errorIndication, errorStatus, errorIndex, varBinds = yield from setCmd(
    ...         SnmpEngine(),
    ...         CommunityData('public'),
    ...         UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...         ContextData(),
    ...         ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0), 'Linux i386')
    ...     )
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> asyncio.get_event_loop().run_until_complete(run())
    (None, 0, 0, [ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('Linux i386'))])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBinds, cbCtx):
        lookupMib, future = cbCtx
        if future.cancelled():
            return
        future.set_result(
            (errorIndication, errorStatus, errorIndex, vbProcessor.unmakeVarBinds(snmpEngine, varBinds, lookupMib))
        )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    future = asyncio.Future()

    cmdgen.SetCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True), future)
    )
    return future


@asyncio.coroutine
def nextCmd(snmpEngine, authData, transportTarget, contextData,
            *varBinds, **options):
    """Creates a generator to perform SNMP GETNEXT query.

    When itereator gets advanced by :py:mod:`asyncio` main loop,
    SNMP GETNEXT request is send (:RFC:`1905#section-4.2.2`).
    The iterator yields :py:class:`asyncio.Future` which gets done whenever
    response arrives or error occurs.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncio.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncio.Udp6TransportTarget`
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
        A sequence of sequences (e.g. 2-D array) of
        :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
        representing a table of MIB variables returned in SNMP response.
        Inner sequences represent table rows and ordered exactly the same
        as `varBinds` in request. Response to GETNEXT always contain
        a single row.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Examples
    --------
    >>> import asyncio
    >>> from pysnmp.hlapi.asyncio import *
    >>>
    >>> @asyncio.coroutine
    ... def run():
    ...     errorIndication, errorStatus, errorIndex, varBinds = yield from nextCmd(
    ...         SnmpEngine(),
    ...         CommunityData('public'),
    ...         UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...         ContextData(),
    ...         ObjectType(ObjectIdentity('SNMPv2-MIB', 'system'))
    ...     )
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> asyncio.get_event_loop().run_until_complete(run())
    (None, 0, 0, [[ObjectType(ObjectIdentity('1.3.6.1.2.1.1.1.0'), DisplayString('Linux i386'))]])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBindTable, cbCtx):
        lookupMib, future = cbCtx
        if future.cancelled():
            return
        future.set_result(
            (errorIndication, errorStatus, errorIndex,
             [vbProcessor.unmakeVarBinds(snmpEngine, varBindTableRow, lookupMib) for varBindTableRow in varBindTable])
        )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    future = asyncio.Future()

    cmdgen.NextCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True), future)
    )
    return future


@asyncio.coroutine
def bulkCmd(snmpEngine, authData, transportTarget, contextData,
            nonRepeaters, maxRepetitions, *varBinds, **options):
    """Creates a generator to perform SNMP GETBULK query.

    When itereator gets advanced by :py:mod:`asyncio` main loop,
    SNMP GETBULK request is send (:RFC:`1905#section-4.2.3`).
    The iterator yields :py:class:`asyncio.Future` which gets done whenever
    response arrives or error occurs.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.asyncio.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.asyncio.Udp6TransportTarget`
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

    Yields
    ------
    errorIndication : str
        True value indicates SNMP engine error.
    errorStatus : str
        True value indicates SNMP PDU error.
    errorIndex : int
        Non-zero value refers to `varBinds[errorIndex-1]`
    varBinds : tuple
        A sequence of sequences (e.g. 2-D array) of
        :py:class:`~pysnmp.smi.rfc1902.ObjectType` class instances
        representing a table of MIB variables returned in SNMP response.
        Inner sequences represent table rows and ordered exactly the same
        as `varBinds` in request. Response to GETNEXT always contain
        a single row.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Examples
    --------
    >>> import asyncio
    >>> from pysnmp.hlapi.asyncio import *
    >>>
    >>> @asyncio.coroutine
    ... def run():
    ...     errorIndication, errorStatus, errorIndex, varBinds = yield from bulkCmd(
    ...         SnmpEngine(),
    ...         CommunityData('public'),
    ...         UdpTransportTarget(('demo.snmplabs.com', 161)),
    ...         ContextData(),
    ...         0, 2,
    ...         ObjectType(ObjectIdentity('SNMPv2-MIB', 'system'))
    ...     )
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    >>>
    >>> asyncio.get_event_loop().run_until_complete(run())
    (None, 0, 0, [[ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.1.0')), DisplayString('SunOS zeus.snmplabs.com 4.1.3_U1 1 sun4m')), ObjectType(ObjectIdentity(ObjectName('1.3.6.1.2.1.1.2.0')), ObjectIdentifier('1.3.6.1.4.1.424242.1.1')]])
    >>>

    """

    def __cbFun(snmpEngine, sendRequestHandle,
                errorIndication, errorStatus, errorIndex,
                varBindTable, cbCtx):
        lookupMib, future = cbCtx
        if future.cancelled():
            return
        future.set_result(
            (errorIndication, errorStatus, errorIndex,
             [vbProcessor.unmakeVarBinds(snmpEngine, varBindTableRow, lookupMib) for varBindTableRow in varBindTable])
        )

    addrName, paramsName = lcd.configure(snmpEngine, authData, transportTarget)

    future = asyncio.Future()

    cmdgen.BulkCommandGenerator().sendVarBinds(
        snmpEngine, addrName, contextData.contextEngineId,
        contextData.contextName, nonRepeaters, maxRepetitions,
        vbProcessor.makeVarBinds(snmpEngine, varBinds), __cbFun,
        (options.get('lookupMib', True), future)
    )
    return future
