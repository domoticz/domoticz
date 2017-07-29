#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
import traceback
from pysnmp.smi import error
from pysnmp import debug

__all__ = ['AbstractMibInstrumController', 'MibInstrumController']


class AbstractMibInstrumController(object):
    def readVars(self, varBinds, acInfo=(None, None)):
        raise error.NoSuchInstanceError(idx=0)

    def readNextVars(self, varBinds, acInfo=(None, None)):
        raise error.EndOfMibViewError(idx=0)

    def writeVars(self, varBinds, acInfo=(None, None)):
        raise error.NoSuchObjectError(idx=0)


class MibInstrumController(AbstractMibInstrumController):
    fsmReadVar = {
        # ( state, status ) -> newState
        ('start', 'ok'): 'readTest',
        ('readTest', 'ok'): 'readGet',
        ('readGet', 'ok'): 'stop',
        ('*', 'err'): 'stop'
    }
    fsmReadNextVar = {
        # ( state, status ) -> newState
        ('start', 'ok'): 'readTestNext',
        ('readTestNext', 'ok'): 'readGetNext',
        ('readGetNext', 'ok'): 'stop',
        ('*', 'err'): 'stop'
    }
    fsmWriteVar = {
        # ( state, status ) -> newState
        ('start', 'ok'): 'writeTest',
        ('writeTest', 'ok'): 'writeCommit',
        ('writeCommit', 'ok'): 'writeCleanup',
        ('writeCleanup', 'ok'): 'readTest',
        # Do read after successful write
        ('readTest', 'ok'): 'readGet',
        ('readGet', 'ok'): 'stop',
        # Error handling
        ('writeTest', 'err'): 'writeCleanup',
        ('writeCommit', 'err'): 'writeUndo',
        ('writeUndo', 'ok'): 'readTest',
        # Ignore read errors (removed columns)
        ('readTest', 'err'): 'stop',
        ('readGet', 'err'): 'stop',
        ('*', 'err'): 'stop'
    }

    def __init__(self, mibBuilder):
        self.mibBuilder = mibBuilder
        self.lastBuildId = -1
        self.lastBuildSyms = {}

    def getMibBuilder(self):
        return self.mibBuilder

    # MIB indexing

    def __indexMib(self):
        # Build a tree from MIB objects found at currently loaded modules
        if self.lastBuildId == self.mibBuilder.lastBuildId:
            return

        (MibScalarInstance, MibScalar, MibTableColumn, MibTableRow,
         MibTable) = self.mibBuilder.importSymbols(
            'SNMPv2-SMI', 'MibScalarInstance', 'MibScalar',
            'MibTableColumn', 'MibTableRow', 'MibTable'
        )

        mibTree, = self.mibBuilder.importSymbols('SNMPv2-SMI', 'iso')

        #
        # Management Instrumentation gets organized as follows:
        #
        # MibTree
        #   |
        #   +----MibScalar
        #   |        |
        #   |        +-----MibScalarInstance
        #   |
        #   +----MibTable
        #   |
        #   +----MibTableRow
        #          |
        #          +-------MibTableColumn
        #                        |
        #                        +------MibScalarInstance(s)
        #
        # Mind you, only Managed Objects get indexed here, various MIB defs and
        # constants can't be SNMP managed so we drop them.
        #
        scalars = {}
        instances = {}
        tables = {}
        rows = {}
        cols = {}

        # Sort by module name to give user a chance to slip-in
        # custom MIB modules (that would be sorted out first)
        mibSymbols = list(self.mibBuilder.mibSymbols.items())
        mibSymbols.sort(key=lambda x: x[0], reverse=True)

        for modName, mibMod in mibSymbols:
            for symObj in mibMod.values():
                if isinstance(symObj, MibTable):
                    tables[symObj.name] = symObj
                elif isinstance(symObj, MibTableRow):
                    rows[symObj.name] = symObj
                elif isinstance(symObj, MibTableColumn):
                    cols[symObj.name] = symObj
                elif isinstance(symObj, MibScalarInstance):
                    instances[symObj.name] = symObj
                elif isinstance(symObj, MibScalar):
                    scalars[symObj.name] = symObj

        # Detach items from each other
        for symName, parentName in self.lastBuildSyms.items():
            if parentName in scalars:
                scalars[parentName].unregisterSubtrees(symName)
            elif parentName in cols:
                cols[parentName].unregisterSubtrees(symName)
            elif parentName in rows:
                rows[parentName].unregisterSubtrees(symName)
            else:
                mibTree.unregisterSubtrees(symName)

        lastBuildSyms = {}

        # Attach Managed Objects Instances to Managed Objects
        for inst in instances.values():
            if inst.typeName in scalars:
                scalars[inst.typeName].registerSubtrees(inst)
            elif inst.typeName in cols:
                cols[inst.typeName].registerSubtrees(inst)
            else:
                raise error.SmiError(
                    'Orphan MIB scalar instance %r at %r' % (inst, self)
                )
            lastBuildSyms[inst.name] = inst.typeName

        # Attach Table Columns to Table Rows
        for col in cols.values():
            rowName = col.name[:-1]  # XXX
            if rowName in rows:
                rows[rowName].registerSubtrees(col)
            else:
                raise error.SmiError(
                    'Orphan MIB table column %r at %r' % (col, self)
                )
            lastBuildSyms[col.name] = rowName

        # Attach Table Rows to MIB tree
        for row in rows.values():
            mibTree.registerSubtrees(row)
            lastBuildSyms[row.name] = mibTree.name

        # Attach Tables to MIB tree
        for table in tables.values():
            mibTree.registerSubtrees(table)
            lastBuildSyms[table.name] = mibTree.name

        # Attach Scalars to MIB tree
        for scalar in scalars.values():
            mibTree.registerSubtrees(scalar)
            lastBuildSyms[scalar.name] = mibTree.name

        self.lastBuildSyms = lastBuildSyms

        self.lastBuildId = self.mibBuilder.lastBuildId

        debug.logger & debug.flagIns and debug.logger('__indexMib: rebuilt')

    # MIB instrumentation

    def flipFlopFsm(self, fsmTable, inputVarBinds, acInfo):
        self.__indexMib()
        debug.logger & debug.flagIns and debug.logger('flipFlopFsm: input var-binds %r' % (inputVarBinds,))
        mibTree, = self.mibBuilder.importSymbols('SNMPv2-SMI', 'iso')
        outputVarBinds = []
        state, status = 'start', 'ok'
        origExc = None
        while True:
            k = (state, status)
            if k in fsmTable:
                fsmState = fsmTable[k]
            else:
                k = ('*', status)
                if k in fsmTable:
                    fsmState = fsmTable[k]
                else:
                    raise error.SmiError(
                        'Unresolved FSM state %s, %s' % (state, status)
                    )
            debug.logger & debug.flagIns and debug.logger(
                'flipFlopFsm: state %s status %s -> fsmState %s' % (state, status, fsmState))
            state = fsmState
            status = 'ok'
            if state == 'stop':
                break
            idx = 0
            for name, val in inputVarBinds:
                f = getattr(mibTree, state, None)
                if f is None:
                    raise error.SmiError(
                        'Unsupported state handler %s at %s' % (state, self)
                    )
                try:
                    # Convert to tuple to avoid ObjectName instantiation
                    # on subscription
                    rval = f(tuple(name), val, idx, acInfo)
                except error.SmiError:
                    exc_t, exc_v, exc_tb = sys.exc_info()
                    debug.logger & debug.flagIns and debug.logger(
                        'flipFlopFsm: fun %s exception %s for %s=%r with traceback: %s' % (
                            f, exc_t, name, val, traceback.format_exception(exc_t, exc_v, exc_tb)))
                    if origExc is None:  # Take the first exception
                        origExc, origTraceback = exc_v, exc_tb
                    status = 'err'
                    break
                else:
                    debug.logger & debug.flagIns and debug.logger(
                        'flipFlopFsm: fun %s suceeded for %s=%r' % (f, name, val))
                    if rval is not None:
                        outputVarBinds.append((rval[0], rval[1]))
                idx += 1
        if origExc:
            if sys.version_info[0] <= 2:
                raise origExc
            else:
                try:
                    raise origExc.with_traceback(origTraceback)
                finally:
                    # Break cycle between locals and traceback object
                    # (seems to be irrelevant on Py3 but just in case)
                    del origTraceback
        return outputVarBinds

    def readVars(self, varBinds, acInfo=(None, None)):
        return self.flipFlopFsm(self.fsmReadVar, varBinds, acInfo)

    def readNextVars(self, varBinds, acInfo=(None, None)):
        return self.flipFlopFsm(self.fsmReadNextVar, varBinds, acInfo)

    def writeVars(self, varBinds, acInfo=(None, None)):
        return self.flipFlopFsm(self.fsmWriteVar, varBinds, acInfo)
