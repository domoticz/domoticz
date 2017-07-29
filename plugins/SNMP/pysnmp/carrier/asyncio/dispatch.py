#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# Copyright (C) 2014, Zebra Technologies
# Authors: Matt Hooks <me@matthooks.com>
#          Zachary Lorusso <zlorusso@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
import sys
import traceback
from pysnmp.carrier.base import AbstractTransportDispatcher
from pysnmp.error import PySnmpError

try:
    import asyncio
except ImportError:
    import trollius as asyncio


class AsyncioDispatcher(AbstractTransportDispatcher):
    """AsyncioDispatcher based on asyncio event loop"""

    def __init__(self, *args, **kwargs):
        AbstractTransportDispatcher.__init__(self)
        self.__transportCount = 0
        if 'timeout' in kwargs:
            self.setTimerResolution(kwargs['timeout'])
        self.loopingcall = None
        self.loop = kwargs.pop('loop', asyncio.get_event_loop())

    @asyncio.coroutine
    def handle_timeout(self):
        while True:
            yield asyncio.From(asyncio.sleep(self.getTimerResolution()))
            self.handleTimerTick(self.loop.time())

    def runDispatcher(self, timeout=0.0):
        if not self.loop.is_running():
            try:
                self.loop.run_forever()
            except KeyboardInterrupt:
                raise
            except Exception:
                raise PySnmpError(';'.join(traceback.format_exception(*sys.exc_info())))

    def registerTransport(self, tDomain, transport):
        if self.loopingcall is None and self.getTimerResolution() > 0:
            self.loopingcall = asyncio.async(self.handle_timeout())
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
        if self.__transportCount == 0 and not self.loopingcall.done():
            self.loopingcall.cancel()
            self.loopingcall = None


# Trollius or Tulip?
if not hasattr(asyncio, "From"):
    exec ("""\
@asyncio.coroutine
def handle_timeout(self):
    while True:
        yield from asyncio.sleep(self.getTimerResolution())
        self.handleTimerTick(self.loop.time())
AsyncioDispatcher.handle_timeout = handle_timeout\
""")
