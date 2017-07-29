#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp import error


class MetaObserver(object):
    """This is a simple facility for exposing internal SNMP Engine
       working details to pysnmp applications. These details are
       basically local scope variables at a fixed point of execution.

       Two modes of operations are offered:
       1. Consumer: app can request an execution point context by execution point ID.
       2. Provider: app can register its callback function (and context) to be invoked
          once execution reaches specified point. All local scope variables
          will be passed to the callback as in #1.

       It's important to realize that execution context is only guaranteed
       to exist to functions that are at the same or deeper level of invocation
       relative to execution point specified.
    """

    def __init__(self):
        self.__observers = {}
        self.__contexts = {}
        self.__execpoints = {}

    def registerObserver(self, cbFun, *execpoints, **kwargs):
        if cbFun in self.__contexts:
            raise error.PySnmpError('duplicate observer %s' % cbFun)
        else:
            self.__contexts[cbFun] = kwargs.get('cbCtx')
        for execpoint in execpoints:
            if execpoint not in self.__observers:
                self.__observers[execpoint] = []
            self.__observers[execpoint].append(cbFun)

    def unregisterObserver(self, cbFun=None):
        if cbFun is None:
            self.__observers.clear()
            self.__contexts.clear()
        else:
            for execpoint in dict(self.__observers):
                if cbFun in self.__observers[execpoint]:
                    self.__observers[execpoint].remove(cbFun)
                if not self.__observers[execpoint]:
                    del self.__observers[execpoint]

    def storeExecutionContext(self, snmpEngine, execpoint, variables):
        self.__execpoints[execpoint] = variables
        if execpoint in self.__observers:
            for cbFun in self.__observers[execpoint]:
                cbFun(snmpEngine, execpoint, variables, self.__contexts[cbFun])

    def clearExecutionContext(self, snmpEngine, *execpoints):
        if execpoints:
            for execpoint in execpoints:
                del self.__execpoints[execpoint]
        else:
            self.__execpoints.clear()

    def getExecutionContext(self, execpoint):
        return self.__execpoints[execpoint]
