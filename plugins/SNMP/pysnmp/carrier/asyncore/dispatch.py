#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from time import time
from sys import exc_info
from traceback import format_exception
from asyncore import socket_map
from asyncore import loop
from pysnmp.carrier.base import AbstractTransportDispatcher
from pysnmp.error import PySnmpError


class AsyncoreDispatcher(AbstractTransportDispatcher):
    def __init__(self):
        self.__sockMap = {}  # use own map for MT safety
        self.timeout = 0.5
        AbstractTransportDispatcher.__init__(self)

    def getSocketMap(self):
        return self.__sockMap

    def setSocketMap(self, sockMap=socket_map):
        self.__sockMap = sockMap

    def registerTransport(self, tDomain, t):
        AbstractTransportDispatcher.registerTransport(self, tDomain, t)
        t.registerSocket(self.__sockMap)

    def unregisterTransport(self, tDomain):
        self.getTransport(tDomain).unregisterSocket(self.__sockMap)
        AbstractTransportDispatcher.unregisterTransport(self, tDomain)

    def transportsAreWorking(self):
        for transport in self.__sockMap.values():
            if transport.writable():
                return 1
        return 0

    def runDispatcher(self, timeout=0.0):
        while self.jobsArePending() or self.transportsAreWorking():
            try:
                loop(timeout and timeout or self.timeout,
                     use_poll=True, map=self.__sockMap, count=1)
            except KeyboardInterrupt:
                raise
            except:
                raise PySnmpError('poll error: %s' % ';'.join(format_exception(*exc_info())))
            self.handleTimerTick(time())
