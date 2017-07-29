#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# Copyright (C) 2014, Zebra Technologies
# Authors: Matt Hooks <me@matthooks.com>
#          Zachary Lorusso <zlorusso@gmail.com>
#
from pysnmp.smi.rfc1902 import *
from pysnmp.hlapi.auth import *
from pysnmp.hlapi.context import *
from pysnmp.hlapi.lcd import *
from pysnmp.hlapi.varbinds import *
from pysnmp.hlapi.asyncio.transport import *
from pysnmp.entity.rfc3413 import ntforg

try:
    import asyncio
except ImportError:
    import trollius as asyncio

__all__ = ['sendNotification']

vbProcessor = NotificationOriginatorVarBinds()
lcd = NotificationOriginatorLcdConfigurator()


@asyncio.coroutine
def sendNotification(snmpEngine, authData, transportTarget, contextData,
                     notifyType, varBinds, **options):
    """Creates a generator to send SNMP notification.

    When itereator gets advanced by :py:mod:`asyncio` main loop,
    SNMP TRAP or INFORM notification is send (:RFC:`1905#section-4.2.6`).
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

    notifyType : str
        Indicates type of notification to be sent. Recognized literal
        values are *trap* or *inform*.

    varBinds: tuple
        Single :py:class:`~pysnmp.smi.rfc1902.NotificationType` class instance
        representing a minimum sequence of MIB variables required for
        particular notification type.
        Alternatively, a sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        objects could be passed instead. In the latter case it is up to
        the user to ensure proper Notification PDU contents.

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
    The `sendNotification` generator will be exhausted immidiately unless
    an instance of :py:class:`~pysnmp.smi.rfc1902.NotificationType` class
    or a sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType` `varBinds`
    are send back into running generator (supported since Python 2.6).

    Examples
    --------
    >>> import asyncio
    >>> from pysnmp.hlapi.asyncio import *
    >>>
    >>> @asyncio.coroutine
    ... def run():
    ...     errorIndication, errorStatus, errorIndex, varBinds = yield from sendNotification(
    ...         SnmpEngine(),
    ...         CommunityData('public'),
    ...         UdpTransportTarget(('demo.snmplabs.com', 162)),
    ...         ContextData(),
    ...         'trap',
    ...         NotificationType(ObjectIdentity('IF-MIB', 'linkDown')))
    ...     print(errorIndication, errorStatus, errorIndex, varBinds)
    ...
    >>> asyncio.get_event_loop().run_until_complete(run())
    (None, 0, 0, [])
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

    notifyName = lcd.configure(
        snmpEngine, authData, transportTarget, notifyType
    )

    future = asyncio.Future()

    ntforg.NotificationOriginator().sendVarBinds(
        snmpEngine,
        notifyName,
        contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds),
        __cbFun,
        (options.get('lookupMib', True), future)
    )

    if notifyType == 'trap':
        def __trapFun(future):
            if future.cancelled():
                return
            future.set_result((None, 0, 0, []))

        loop = asyncio.get_event_loop()
        loop.call_soon(__trapFun, future)

    return future
