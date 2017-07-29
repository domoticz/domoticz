#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# All code in this file belongs to obsolete, compatibility wrappers.
# Never use interfaces below for new applications!
#
from pysnmp.hlapi.asyncore import *
from pysnmp.hlapi.asyncore import sync
from pysnmp.hlapi.varbinds import *
from pysnmp.hlapi.lcd import *
from pyasn1.compat.octets import null
from pysnmp.entity import config
from pysnmp.entity.rfc3413 import context

__all__ = ['AsynNotificationOriginator',
           'NotificationOriginator',
           'MibVariable']

MibVariable = ObjectIdentity


class ErrorIndicationReturn(object):
    def __init__(self, *vars):
        self.__vars = vars

    def __getitem__(self, i):
        return self.__vars[i]

    def __nonzero__(self):
        return bool(self)

    def __bool__(self):
        return bool(self.__vars[0])

    def __str__(self):
        return str(self.__vars[0])


class AsynNotificationOriginator(object):
    vbProcessor = NotificationOriginatorVarBinds()
    lcd = NotificationOriginatorLcdConfigurator()

    def __init__(self, snmpEngine=None, snmpContext=None):
        if snmpEngine is None:
            self.snmpEngine = snmpEngine = SnmpEngine()
        else:
            self.snmpEngine = snmpEngine

        if snmpContext is None:
            self.snmpContext = context.SnmpContext(self.snmpEngine)
            config.addContext(
                self.snmpEngine, ''  # this is leaky
            )
        else:
            self.snmpContext = snmpContext

        self.mibViewController = self.vbProcessor.getMibViewController(self.snmpEngine)

    def __del__(self):
        self.uncfgNtfOrg()

    def cfgNtfOrg(self, authData, transportTarget, notifyType):
        return self.lcd.configure(
            self.snmpEngine, authData, transportTarget, notifyType
        )

    def uncfgNtfOrg(self, authData=None):
        return self.lcd.unconfigure(self.snmpEngine, authData)

    def makeVarBinds(self, varBinds):
        return self.vbProcessor.makeVarBinds(
            self.snmpEngine, varBinds
        )

    def unmakeVarBinds(self, varBinds, lookupNames, lookupValues):
        return self.vbProcessor.unmakeVarBinds(
            self.snmpEngine, varBinds, lookupNames or lookupValues
        )

    def sendNotification(self, authData, transportTarget,
                         notifyType, notificationType,
                         varBinds=(),  # legacy, use NotificationType instead
                         cbInfo=(None, None),
                         lookupNames=False, lookupValues=False,
                         contextEngineId=None,  # XXX ordering incompatibility
                         contextName=null):

        def __cbFun(snmpEngine, sendRequestHandle, errorIndication,
                    errorStatus, errorIndex, varBinds, cbCtx):
            cbFun, cbCtx = cbCtx
            try:
                # we need to pass response PDU information to user for INFORMs
                return cbFun and cbFun(
                    sendRequestHandle,
                    errorIndication,
                    errorStatus, errorIndex,
                    varBinds,
                    cbCtx
                )
            except TypeError:
                # a backward compatible way of calling user function
                return cbFun(
                    sendRequestHandle,
                    errorIndication,
                    cbCtx
                )

        # for backward compatibility
        if contextName is null and authData.contextName:
            contextName = authData.contextName

        if not isinstance(notificationType,
                          (ObjectIdentity, ObjectType, NotificationType)):
            if isinstance(notificationType[0], tuple):
                # legacy
                notificationType = ObjectIdentity(notificationType[0][0], notificationType[0][1], *notificationType[1:])
            else:
                notificationType = ObjectIdentity(notificationType)

        if not isinstance(notificationType, NotificationType):
            notificationType = NotificationType(notificationType)

        return sendNotification(
            self.snmpEngine,
            authData, transportTarget,
            ContextData(contextEngineId or self.snmpContext.contextEngineId,
                        contextName),
            notifyType, notificationType.addVarBinds(*varBinds),
            __cbFun,
            cbInfo,
            lookupNames or lookupValues
        )

    asyncSendNotification = sendNotification


class NotificationOriginator(object):
    vbProcessor = NotificationOriginatorVarBinds()

    def __init__(self, snmpEngine=None, snmpContext=None, asynNtfOrg=None):
        # compatibility attributes
        self.snmpEngine = snmpEngine or SnmpEngine()
        self.mibViewController = self.vbProcessor.getMibViewController(self.snmpEngine)

    # the varBinds parameter is legacy, use NotificationType instead

    def sendNotification(self, authData, transportTarget, notifyType,
                         notificationType, *varBinds, **kwargs):
        if 'lookupNames' not in kwargs:
            kwargs['lookupNames'] = False
        if 'lookupValues' not in kwargs:
            kwargs['lookupValues'] = False
        if not isinstance(notificationType,
                          (ObjectIdentity, ObjectType, NotificationType)):
            if isinstance(notificationType[0], tuple):
                # legacy
                notificationType = ObjectIdentity(notificationType[0][0], notificationType[0][1], *notificationType[1:])
            else:
                notificationType = ObjectIdentity(notificationType)

        if not isinstance(notificationType, NotificationType):
            notificationType = NotificationType(notificationType)

        for (errorIndication,
             errorStatus,
             errorIndex,
             rspVarBinds) in sync.sendNotification(self.snmpEngine, authData,
                                                   transportTarget,
                                                   ContextData(kwargs.get('contextEngineId'),
                                                               kwargs.get('contextName', null)),
                                                   notifyType,
                                                   notificationType.addVarBinds(*varBinds),
                                                   **kwargs):
            if notifyType == 'inform':
                return errorIndication, errorStatus, errorIndex, rspVarBinds
            else:
                break
