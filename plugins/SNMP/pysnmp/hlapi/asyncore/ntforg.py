#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.smi.rfc1902 import *
from pysnmp.entity.rfc3413 import ntforg
from pysnmp.hlapi.auth import *
from pysnmp.hlapi.context import *
from pysnmp.hlapi.lcd import *
from pysnmp.hlapi.varbinds import *
from pysnmp.hlapi.asyncore.transport import *

__all__ = ['sendNotification']

vbProcessor = NotificationOriginatorVarBinds()
lcd = NotificationOriginatorLcdConfigurator()


def sendNotification(snmpEngine, authData, transportTarget, contextData,
                     notifyType, varBinds, cbFun=None, cbCtx=None,
                     lookupMib=False):
    """Send SNMP notification.

    Based on passed parameters, prepares SNMP TRAP or INFORM
    notification (:RFC:`1905#section-4.2.6`) and schedules its
    transmission by I/O framework at a later point of time.

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

    notifyType : str
        Indicates type of notification to be sent. Recognized literal
        values are *trap* or *inform*.

    varBinds : tuple
        Single :py:class:`~pysnmp.smi.rfc1902.NotificationType` class
        instance representing a minimum sequence of MIB variables
        required for particular notification type. Alternatively,
        a sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType`
        objects could be passed instead. In the latter case it is up to
        the user to ensure proper Notification PDU contents.

    Other Parameters
    ----------------
    cbFun : callable
        user-supplied callable that is invoked to pass SNMP response
        to *INFORM* notification or error to user at a later point of
        time. The `cbFun` callable is never invoked for *TRAP* notifications.
    cbCtx : object
        user-supplied object passing additional parameters to/from
        `cbFun`.
    lookupMib : bool
        `lookupMib` - load MIB and resolve response MIB variables at
        the cost of slightly reduced performance. Default is `True`.

    Notes
    -----
    User-supplied `cbFun` callable must have the following call
    signature:

    * snmpEngine (:py:class:`~pysnmp.hlapi.SnmpEngine`):
      Class instance representing SNMP engine.
    * sendRequestHandle (int): Unique request identifier. Can be used
      for matching multiple ongoing *INFORM* notifications with received
      responses.
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
        responses with ongoing *INFORM* requests. Returns `None` for
        *TRAP* notifications.

    Raises
    ------
    PySnmpError
        Or its derivative indicating that an error occurred while
        performing SNMP operation.

    Examples
    --------
    >>> from pysnmp.hlapi.asyncore import *
    >>>
    >>> snmpEngine = SnmpEngine()
    >>> sendNotification(
    ...     snmpEngine,
    ...     CommunityData('public'),
    ...     UdpTransportTarget(('demo.snmplabs.com', 162)),
    ...     ContextData(),
    ...     'trap',
    ...     NotificationType(ObjectIdentity('SNMPv2-MIB', 'coldStart')),
    ... )
    >>> snmpEngine.transportDispatcher.runDispatcher()
    >>>

    """

    # noinspection PyShadowingNames
    def __cbFun(snmpEngine, sendRequestHandle, errorIndication,
                errorStatus, errorIndex, varBinds, cbCtx):
        lookupMib, cbFun, cbCtx = cbCtx
        return cbFun and cbFun(
            snmpEngine, sendRequestHandle, errorIndication,
            errorStatus, errorIndex,
            vbProcessor.unmakeVarBinds(
                snmpEngine, varBinds, lookupMib
            ), cbCtx
        )

    notifyName = lcd.configure(snmpEngine, authData, transportTarget,
                               notifyType)

    return ntforg.NotificationOriginator().sendVarBinds(
        snmpEngine, notifyName,
        contextData.contextEngineId, contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds),
        __cbFun, (lookupMib, cbFun, cbCtx)
    )
