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
from pyasn1.type import univ

__all__ = ['AsynCommandGenerator', 'CommandGenerator', 'MibVariable']

MibVariable = ObjectIdentity


class AsynCommandGenerator(object):
    _null = univ.Null('')

    vbProcessor = CommandGeneratorVarBinds()
    lcd = CommandGeneratorLcdConfigurator()

    def __init__(self, snmpEngine=None):
        if snmpEngine is None:
            self.snmpEngine = SnmpEngine()
        else:
            self.snmpEngine = snmpEngine

        self.mibViewController = self.vbProcessor.getMibViewController(self.snmpEngine)

    def __del__(self):
        self.lcd.unconfigure(self.snmpEngine)

    def cfgCmdGen(self, authData, transportTarget):
        return self.lcd.configure(self.snmpEngine, authData, transportTarget)

    def uncfgCmdGen(self, authData=None):
        return self.lcd.unconfigure(self.snmpEngine, authData)

    # compatibility stub
    def makeReadVarBinds(self, varNames):
        return self.makeVarBinds([(x, self._null) for x in varNames])

    def makeVarBinds(self, varBinds):
        return self.vbProcessor.makeVarBinds(self.snmpEngine, varBinds)

    def unmakeVarBinds(self, varBinds, lookupNames, lookupValues):
        return self.vbProcessor.unmakeVarBinds(
            self.snmpEngine, varBinds, lookupNames or lookupValues
        )

    def getCmd(self, authData, transportTarget, varNames, cbInfo,
               lookupNames=False, lookupValues=False,
               contextEngineId=None, contextName=null):

        def __cbFun(snmpEngine, sendRequestHandle,
                    errorIndication, errorStatus, errorIndex,
                    varBindTable, cbInfo):
            cbFun, cbCtx = cbInfo
            cbFun(sendRequestHandle,
                  errorIndication, errorStatus, errorIndex,
                  varBindTable, cbCtx)

        # for backward compatibility
        if contextName is null and authData.contextName:
            contextName = authData.contextName

        return getCmd(
            self.snmpEngine, authData, transportTarget,
            ContextData(contextEngineId, contextName),
            *[(x, self._null) for x in varNames],
            **dict(cbFun=__cbFun, cbCtx=cbInfo,
                   lookupMib=lookupNames or lookupValues)
        )

    asyncGetCmd = getCmd

    def setCmd(self, authData, transportTarget, varBinds, cbInfo,
               lookupNames=False, lookupValues=False,
               contextEngineId=None, contextName=null):

        def __cbFun(snmpEngine, sendRequestHandle,
                    errorIndication, errorStatus, errorIndex,
                    varBindTable, cbInfo):
            cbFun, cbCtx = cbInfo
            cbFun(sendRequestHandle,
                  errorIndication, errorStatus, errorIndex,
                  varBindTable, cbCtx)

        # for backward compatibility
        if contextName is null and authData.contextName:
            contextName = authData.contextName

        return setCmd(
            self.snmpEngine, authData, transportTarget,
            ContextData(contextEngineId, contextName), *varBinds,
            **dict(cbFun=__cbFun, cbCtx=cbInfo,
                   lookupMib=lookupNames or lookupValues)
        )

    asyncSetCmd = setCmd

    def nextCmd(self, authData, transportTarget, varNames, cbInfo,
                lookupNames=False, lookupValues=False,
                contextEngineId=None, contextName=null):

        def __cbFun(snmpEngine, sendRequestHandle,
                    errorIndication, errorStatus, errorIndex,
                    varBindTable, cbInfo):
            cbFun, cbCtx = cbInfo
            return cbFun(sendRequestHandle,
                         errorIndication, errorStatus, errorIndex,
                         varBindTable, cbCtx)

        # for backward compatibility
        if contextName is null and authData.contextName:
            contextName = authData.contextName

        return nextCmd(
            self.snmpEngine, authData, transportTarget,
            ContextData(contextEngineId, contextName),
            *[(x, self._null) for x in varNames],
            **dict(cbFun=__cbFun, cbCtx=cbInfo,
                   lookupMib=lookupNames or lookupValues)
        )

    asyncNextCmd = nextCmd

    def bulkCmd(self, authData, transportTarget,
                nonRepeaters, maxRepetitions, varNames, cbInfo,
                lookupNames=False, lookupValues=False,
                contextEngineId=None, contextName=null):

        def __cbFun(snmpEngine, sendRequestHandle,
                    errorIndication, errorStatus, errorIndex,
                    varBindTable, cbInfo):
            cbFun, cbCtx = cbInfo
            return cbFun(sendRequestHandle,
                         errorIndication, errorStatus, errorIndex,
                         varBindTable, cbCtx)

        # for backward compatibility
        if contextName is null and authData.contextName:
            contextName = authData.contextName

        return bulkCmd(
            self.snmpEngine, authData, transportTarget,
            ContextData(contextEngineId, contextName),
            nonRepeaters, maxRepetitions,
            *[(x, self._null) for x in varNames],
            **dict(cbFun=__cbFun, cbCtx=cbInfo,
                   lookupMib=lookupNames or lookupValues)
        )

    asyncBulkCmd = bulkCmd


class CommandGenerator(object):
    _null = univ.Null('')

    def __init__(self, snmpEngine=None, asynCmdGen=None):
        # compatibility attributes
        self.snmpEngine = snmpEngine or SnmpEngine()

    def getCmd(self, authData, transportTarget, *varNames, **kwargs):
        if 'lookupNames' not in kwargs:
            kwargs['lookupNames'] = False
        if 'lookupValues' not in kwargs:
            kwargs['lookupValues'] = False
        errorIndication, errorStatus, errorIndex, varBinds = None, 0, 0, []
        for (errorIndication,
             errorStatus,
             errorIndex,
             varBinds) in sync.getCmd(self.snmpEngine, authData, transportTarget,
                                      ContextData(kwargs.get('contextEngineId'),
                                                  kwargs.get('contextName', null)),
                                      *[(x, self._null) for x in varNames],
                                      **kwargs):
            break
        return errorIndication, errorStatus, errorIndex, varBinds

    def setCmd(self, authData, transportTarget, *varBinds, **kwargs):
        if 'lookupNames' not in kwargs:
            kwargs['lookupNames'] = False
        if 'lookupValues' not in kwargs:
            kwargs['lookupValues'] = False
        errorIndication, errorStatus, errorIndex, rspVarBinds = None, 0, 0, []
        for (errorIndication,
             errorStatus,
             errorIndex,
             rspVarBinds) in sync.setCmd(self.snmpEngine, authData, transportTarget,
                                         ContextData(kwargs.get('contextEngineId'),
                                                     kwargs.get('contextName', null)),
                                         *varBinds,
                                         **kwargs):
            break

        return errorIndication, errorStatus, errorIndex, rspVarBinds

    def nextCmd(self, authData, transportTarget, *varNames, **kwargs):
        if 'lookupNames' not in kwargs:
            kwargs['lookupNames'] = False
        if 'lookupValues' not in kwargs:
            kwargs['lookupValues'] = False
        if 'lexicographicMode' not in kwargs:
            kwargs['lexicographicMode'] = False
        errorIndication, errorStatus, errorIndex = None, 0, 0
        varBindTable = []
        for (errorIndication,
             errorStatus,
             errorIndex,
             varBinds) in sync.nextCmd(self.snmpEngine, authData, transportTarget,
                                       ContextData(kwargs.get('contextEngineId'),
                                                   kwargs.get('contextName', null)),
                                       *[(x, self._null) for x in varNames],
                                       **kwargs):
            if errorIndication or errorStatus:
                return errorIndication, errorStatus, errorIndex, varBinds

            varBindTable.append(varBinds)

        return errorIndication, errorStatus, errorIndex, varBindTable

    def bulkCmd(self, authData, transportTarget,
                nonRepeaters, maxRepetitions, *varNames, **kwargs):
        if 'lookupNames' not in kwargs:
            kwargs['lookupNames'] = False
        if 'lookupValues' not in kwargs:
            kwargs['lookupValues'] = False
        if 'lexicographicMode' not in kwargs:
            kwargs['lexicographicMode'] = False
        errorIndication, errorStatus, errorIndex = None, 0, 0
        varBindTable = []
        for (errorIndication,
             errorStatus,
             errorIndex,
             varBinds) in sync.bulkCmd(self.snmpEngine, authData,
                                       transportTarget,
                                       ContextData(kwargs.get('contextEngineId'),
                                                   kwargs.get('contextName', null)),
                                       nonRepeaters, maxRepetitions,
                                       *[(x, self._null) for x in varNames],
                                       **kwargs):
            if errorIndication or errorStatus:
                return errorIndication, errorStatus, errorIndex, varBinds

            varBindTable.append(varBinds)

        return errorIndication, errorStatus, errorIndex, varBindTable
