#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# This is a Python 2.6- version of the same file at level up
#
from pysnmp.hlapi.asyncore import cmdgen
from pysnmp.hlapi.varbinds import *
from pysnmp.proto.rfc1905 import endOfMibView
from pysnmp.proto import errind
from pyasn1.type.univ import Null

__all__ = ['getCmd', 'nextCmd', 'setCmd', 'bulkCmd', 'next']


# noinspection PyShadowingBuiltins
def next(iter):
    return iter.next()

def getCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
    # noinspection PyShadowingNames
    def cbFun(snmpEngine, sendRequestHandle,
              errorIndication, errorStatus, errorIndex,
              varBinds, cbCtx):
        cbCtx['errorIndication'] = errorIndication
        cbCtx['errorStatus'] = errorStatus
        cbCtx['errorIndex'] = errorIndex
        cbCtx['varBinds'] = varBinds

    cbCtx = {}

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

    yield errorIndication, errorStatus, errorIndex, varBinds


def setCmd(snmpEngine, authData, transportTarget, contextData,
           *varBinds, **options):
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
        cmdgen.setCmd(snmpEngine, authData, transportTarget,
                      contextData, *varBinds,
                      **dict(cbFun=cbFun, cbCtx=cbCtx,
                             lookupMib=options.get('lookupMib', True)))

        snmpEngine.transportDispatcher.runDispatcher()

        yield (cbCtx['errorIndication'],
               cbCtx['errorStatus'], cbCtx['errorIndex'],
               cbCtx['varBinds'])

        if cbCtx['errorIndication'] != errind.requestTimedOut:
            break


def nextCmd(snmpEngine, authData, transportTarget, contextData,
            *varBinds, **options):
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
        cmdgen.nextCmd(snmpEngine, authData, transportTarget, contextData,
                       *[(x[0], Null()) for x in varBinds],
                       **dict(cbFun=cbFun, cbCtx=cbCtx,
                              lookupMib=options.get('lookupMib', True)))

        snmpEngine.transportDispatcher.runDispatcher()

        errorIndication = cbCtx['errorIndication']
        errorStatus = cbCtx['errorStatus']
        errorIndex = cbCtx['errorIndex']

        if ignoreNonIncreasingOid and errorIndication and \
                isinstance(errorIndication, errind.OidNotIncreasing):
            errorIndication = None

        if errorIndication:
            yield errorIndication, errorStatus, errorIndex, varBinds
            if errorIndication != errind.requestTimedOut:
                return
        elif errorStatus:
            if errorStatus == 2:
                # Hide SNMPv1 noSuchName error which leaks in here
                # from SNMPv1 Agent through internal pysnmp proxy.
                errorStatus = errorStatus.clone(0)
                errorIndex = errorIndex.clone(0)
            yield errorIndication, errorStatus, errorIndex, varBinds
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

            yield errorIndication, errorStatus, errorIndex, varBinds

            if maxRows and totalRows >= maxRows or \
                    maxCalls and totalCalls >= maxCalls:
                return


def bulkCmd(snmpEngine, authData, transportTarget, contextData,
            nonRepeaters, maxRepetitions, *varBinds, **options):
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
            yield (errorIndication, errorStatus, errorIndex,
                   varBindTable and varBindTable[0] or [])
            return
        else:
            for i in range(len(varBindTable)):
                stopFlag = True
                if len(varBindTable[i]) != len(initialVars):
                    varBindTable = i and varBindTable[:i - 1] or []
                    break
                for j in range(len(varBindTable[i])):
                    name, val = varBindTable[i][j]
                    if nullVarBinds[j]:
                        varBindTable[i][j] = name, endOfMibView
                        continue
                    stopFlag = False
                    if isinstance(val, Null):
                        nullVarBinds[j] = True
                    elif not lexicographicMode and \
                            not initialVars[j].isPrefixOf(name):
                        varBindTable[i][j] = name, endOfMibView
                        nullVarBinds[j] = True
                if stopFlag:
                    varBindTable = i and varBindTable[:i - 1] or []
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
                yield errorIndication, errorStatus, errorIndex, varBinds
