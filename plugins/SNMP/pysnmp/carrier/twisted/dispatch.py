#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# Copyright (C) 2008 Truelite Srl <info@truelite.it>
# Author: Filippo Giunchedi <filippo@truelite.it>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the BSD 2-Clause License as shipped with pysnmp.
#
# Description: Transport dispatcher based on twisted.internet.reactor
#
import sys, time, traceback
from twisted.internet import reactor, task
from pysnmp.carrier.base import AbstractTransportDispatcher
from pysnmp.error import PySnmpError


class TwistedDispatcher(AbstractTransportDispatcher):
    """TransportDispatcher based on twisted.internet.reactor"""

    def __init__(self, *args, **kwargs):
        AbstractTransportDispatcher.__init__(self)
        self.__transportCount = 0
        if 'timeout' in kwargs:
            self.setTimerResolution(kwargs['timeout'])
        self.loopingcall = task.LoopingCall(
            lambda self=self: self.handleTimerTick(time.time())
        )

    def runDispatcher(self, timeout=0.0):
        if not reactor.running:
            try:
                reactor.run()
            except KeyboardInterrupt:
                raise
            except:
                raise PySnmpError('reactor error: %s' % ';'.join(traceback.format_exception(*sys.exc_info())))

    # jobstarted/jobfinished might be okay as-is

    def registerTransport(self, tDomain, transport):
        if not self.loopingcall.running and self.getTimerResolution() > 0:
            self.loopingcall.start(self.getTimerResolution(), now=False)
        AbstractTransportDispatcher.registerTransport(
            self, tDomain, transport
        )
        self.__transportCount += 1

    def unregisterTransport(self, tDomain):
        t = AbstractTransportDispatcher.getTransport(self, tDomain)
        if t is not None:
            AbstractTransportDispatcher.unregisterTransport(self, tDomain)
            self.__transportCount -= 1

        # The last transport has been removed, stop the timeout
        if self.__transportCount == 0 and self.loopingcall.running:
            self.loopingcall.stop()
