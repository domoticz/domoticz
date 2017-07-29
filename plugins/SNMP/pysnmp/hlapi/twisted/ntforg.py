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
from pysnmp.entity.rfc3413 import ntforg
from twisted.internet import reactor
from twisted.internet.defer import Deferred
from twisted.python.failure import Failure

__all__ = ['sendNotification']

vbProcessor = NotificationOriginatorVarBinds()
lcd = NotificationOriginatorLcdConfigurator()

def sendNotification(snmpEngine, authData, transportTarget, contextData,
                     notifyType, varBinds, **options):
    """Sends SNMP notification.

    Based on passed parameters, prepares SNMP TRAP or INFORM message
    (:RFC:`1905#section-4.2.6`) and schedules its transmission by
    :mod:`twisted` I/O framework at a later point of time.

    Parameters
    ----------
    snmpEngine : :py:class:`~pysnmp.hlapi.SnmpEngine`
        Class instance representing SNMP engine.

    authData : :py:class:`~pysnmp.hlapi.CommunityData` or :py:class:`~pysnmp.hlapi.UsmUserData`
        Class instance representing SNMP credentials.

    transportTarget : :py:class:`~pysnmp.hlapi.twisted.UdpTransportTarget` or :py:class:`~pysnmp.hlapi.twisted.Udp6TransportTarget`
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
    ...     d = sendNotification(SnmpEngine(),
    ...                          CommunityData('public'),
    ...                          UdpTransportTarget(('demo.snmplabs.com', 162)),
    ...                          ContextData(),
    ...                          'trap',
    ...                          NotificationType(ObjectIdentity('IF-MIB', 'linkDown')))
    ...     d.addCallback(success).addErrback(failure)
    ...     return d
    ...
    >>> react(run)
    (0, 0, [])
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

    notifyName = lcd.configure(
        snmpEngine, authData, transportTarget, notifyType
    )

    def __trapFun(deferred):
        deferred.callback((0, 0, []))

    deferred = Deferred()

    ntforg.NotificationOriginator().sendVarBinds(
        snmpEngine,
        notifyName,
        contextData.contextEngineId,
        contextData.contextName,
        vbProcessor.makeVarBinds(snmpEngine, varBinds),
        __cbFun,
        (options.get('lookupMib', True), deferred)
    )

    if notifyType == 'trap':
        reactor.callLater(0, __trapFun, deferred)

    return deferred
