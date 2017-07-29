#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.carrier import error


class TimerCallable(object):
    def __init__(self, cbFun, callInterval):
        self.__cbFun = cbFun
        self.__callInterval = callInterval
        self.__nextCall = 0

    def __call__(self, timeNow):
        if self.__nextCall <= timeNow:
            self.__cbFun(timeNow)
            self.__nextCall = timeNow + self.__callInterval

    def __eq__(self, cbFun):
        return self.__cbFun == cbFun

    def __ne__(self, cbFun):
        return self.__cbFun != cbFun

    def __lt__(self, cbFun):
        return self.__cbFun < cbFun

    def __le__(self, cbFun):
        return self.__cbFun <= cbFun

    def __gt__(self, cbFun):
        return self.__cbFun > cbFun

    def __ge__(self, cbFun):
        return self.__cbFun >= cbFun


class AbstractTransportDispatcher(object):
    def __init__(self):
        self.__transports = {}
        self.__transportDomainMap = {}
        self.__jobs = {}
        self.__recvCallables = {}
        self.__timerCallables = []
        self.__ticks = 0
        self.__timerResolution = 0.5
        self.__timerDelta = self.__timerResolution * 0.05
        self.__nextTime = 0
        self.__routingCbFun = None

    def _cbFun(self, incomingTransport, transportAddress, incomingMessage):
        if incomingTransport in self.__transportDomainMap:
            transportDomain = self.__transportDomainMap[incomingTransport]
        else:
            raise error.CarrierError(
                'Unregistered transport %s' % (incomingTransport,)
            )

        if self.__routingCbFun:
            recvId = self.__routingCbFun(
                transportDomain, transportAddress, incomingMessage
            )
        else:
            recvId = None

        if recvId in self.__recvCallables:
            self.__recvCallables[recvId](
                self, transportDomain, transportAddress, incomingMessage
            )
        else:
            raise error.CarrierError(
                'No callback for "%r" found - loosing incoming event' % (recvId,)
            )

    # Dispatcher API

    def registerRoutingCbFun(self, routingCbFun):
        if self.__routingCbFun:
            raise error.CarrierError(
                'Data routing callback already registered'
            )
        self.__routingCbFun = routingCbFun

    def unregisterRoutingCbFun(self):
        if self.__routingCbFun:
            self.__routingCbFun = None

    def registerRecvCbFun(self, recvCb, recvId=None):
        if recvId in self.__recvCallables:
            raise error.CarrierError(
                'Receive callback %r already registered' % (recvId is None and '<default>' or recvId,)
            )
        self.__recvCallables[recvId] = recvCb

    def unregisterRecvCbFun(self, recvId=None):
        if recvId in self.__recvCallables:
            del self.__recvCallables[recvId]

    def registerTimerCbFun(self, timerCbFun, tickInterval=None):
        if not tickInterval:
            tickInterval = self.__timerResolution
        self.__timerCallables.append(TimerCallable(timerCbFun, tickInterval))

    def unregisterTimerCbFun(self, timerCbFun=None):
        if timerCbFun:
            self.__timerCallables.remove(timerCbFun)
        else:
            self.__timerCallables = []

    def registerTransport(self, tDomain, transport):
        if tDomain in self.__transports:
            raise error.CarrierError(
                'Transport %s already registered' % (tDomain,)
            )
        transport.registerCbFun(self._cbFun)
        self.__transports[tDomain] = transport
        self.__transportDomainMap[transport] = tDomain

    def unregisterTransport(self, tDomain):
        if tDomain not in self.__transports:
            raise error.CarrierError(
                'Transport %s not registered' % (tDomain,)
            )
        self.__transports[tDomain].unregisterCbFun()
        del self.__transportDomainMap[self.__transports[tDomain]]
        del self.__transports[tDomain]

    def getTransport(self, transportDomain):
        if transportDomain in self.__transports:
            return self.__transports[transportDomain]
        raise error.CarrierError(
            'Transport %s not registered' % (transportDomain,)
        )

    def sendMessage(self, outgoingMessage, transportDomain,
                    transportAddress):
        if transportDomain in self.__transports:
            self.__transports[transportDomain].sendMessage(
                outgoingMessage, transportAddress
            )
        else:
            raise error.CarrierError(
                'No suitable transport domain for %s' % (transportDomain,)
            )

    def getTimerResolution(self):
        return self.__timerResolution

    def setTimerResolution(self, timerResolution):
        if timerResolution < 0.01 or timerResolution > 10:
            raise error.CarrierError('Impossible timer resolution')
        self.__timerResolution = timerResolution
        self.__timerDelta = timerResolution * 0.05

    def getTimerTicks(self):
        return self.__ticks

    def handleTimerTick(self, timeNow):
        if self.__nextTime == 0:  # initial initialization
            self.__nextTime = timeNow + self.__timerResolution - self.__timerDelta

        if self.__nextTime >= timeNow:
            return

        self.__ticks += 1
        self.__nextTime = timeNow + self.__timerResolution - self.__timerDelta

        for timerCallable in self.__timerCallables:
            timerCallable(timeNow)

    def jobStarted(self, jobId, count=1):
        if jobId in self.__jobs:
            self.__jobs[jobId] += count
        else:
            self.__jobs[jobId] = count

    def jobFinished(self, jobId, count=1):
        self.__jobs[jobId] -= count
        if self.__jobs[jobId] == 0:
            del self.__jobs[jobId]

    def jobsArePending(self):
        return bool(self.__jobs)

    def runDispatcher(self, timeout=0.0):
        raise error.CarrierError('Method not implemented')

    def closeDispatcher(self):
        for tDomain in list(self.__transports):
            self.__transports[tDomain].closeTransport()
            self.unregisterTransport(tDomain)
        self.__transports.clear()
        self.unregisterRecvCbFun()
        self.unregisterTimerCbFun()


class AbstractTransportAddress(object):
    _localAddress = None

    def setLocalAddress(self, s):
        self._localAddress = s
        return self

    def getLocalAddress(self):
        return self._localAddress

    def clone(self, localAddress=None):
        return self.__class__(self).setLocalAddress(localAddress is None and self.getLocalAddress() or localAddress)


class AbstractTransport(object):
    protoTransportDispatcher = None
    addressType = AbstractTransportAddress
    _cbFun = None

    @classmethod
    def isCompatibleWithDispatcher(cls, transportDispatcher):
        return isinstance(transportDispatcher, cls.protoTransportDispatcher)

    def registerCbFun(self, cbFun):
        if self._cbFun:
            raise error.CarrierError(
                'Callback function %s already registered at %s' % (self._cbFun, self)
            )
        self._cbFun = cbFun

    def unregisterCbFun(self):
        self._cbFun = None

    def closeTransport(self):
        self.unregisterCbFun()

    # Public API

    def openClientMode(self, iface=None):
        raise error.CarrierError('Method not implemented')

    def openServerMode(self, iface):
        raise error.CarrierError('Method not implemented')

    def sendMessage(self, outgoingMessage, transportAddress):
        raise error.CarrierError('Method not implemented')
