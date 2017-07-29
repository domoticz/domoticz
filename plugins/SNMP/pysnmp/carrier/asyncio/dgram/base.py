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
from pysnmp.carrier.asyncio.base import AbstractAsyncioTransport
from pysnmp.carrier import error
from pysnmp import debug

try:
    import asyncio
except ImportError:
    import trollius as asyncio


class DgramAsyncioProtocol(asyncio.DatagramProtocol, AbstractAsyncioTransport):
    """Base Asyncio datagram Transport, to be used with AsyncioDispatcher"""
    sockFamily = None
    addressType = lambda x: x
    transport = None

    def __init__(self, sock=None, sockMap=None, loop=None):
        self._writeQ = []
        self._lport = None
        if loop is None:
            loop = asyncio.get_event_loop()
        self.loop = loop

    def datagram_received(self, datagram, transportAddress):
        if self._cbFun is None:
            raise error.CarrierError('Unable to call cbFun')
        else:
            self.loop.call_soon(self._cbFun, self, transportAddress, datagram)

    def connection_made(self, transport):
        self.transport = transport
        debug.logger & debug.flagIO and debug.logger('connection_made: invoked')
        while self._writeQ:
            outgoingMessage, transportAddress = self._writeQ.pop(0)
            debug.logger & debug.flagIO and debug.logger('connection_made: transportAddress %r outgoingMessage %s' %
                                                         (transportAddress, debug.hexdump(outgoingMessage)))
            try:
                self.transport.sendto(outgoingMessage, self.normalizeAddress(transportAddress))
            except Exception:
                raise error.CarrierError(';'.join(traceback.format_exception(*sys.exc_info())))

    def connection_lost(self, exc):
        debug.logger & debug.flagIO and debug.logger('connection_lost: invoked')

    # AbstractAsyncioTransport API

    def openClientMode(self, iface=None):
        try:
            c = self.loop.create_datagram_endpoint(
                lambda: self, local_addr=iface, family=self.sockFamily
            )
            self._lport = asyncio.async(c)
        except Exception:
            raise error.CarrierError(';'.join(traceback.format_exception(*sys.exc_info())))
        return self

    def openServerMode(self, iface):
        try:
            c = self.loop.create_datagram_endpoint(
                lambda: self, local_addr=iface, family=self.sockFamily
            )
            self._lport = asyncio.async(c)
        except Exception:
            raise error.CarrierError(';'.join(traceback.format_exception(*sys.exc_info())))
        return self

    def closeTransport(self):
        if self._lport is not None:
            self._lport.cancel()
        if self.transport is not None:
            self.transport.close()
        AbstractAsyncioTransport.closeTransport(self)

    def sendMessage(self, outgoingMessage, transportAddress):
        debug.logger & debug.flagIO and debug.logger('sendMessage: %s transportAddress %r outgoingMessage %s' % (
            (self.transport is None and "queuing" or "sending"),
            transportAddress, debug.hexdump(outgoingMessage)
        ))
        if self.transport is None:
            self._writeQ.append((outgoingMessage, transportAddress))
        else:
            try:
                self.transport.sendto(outgoingMessage, self.normalizeAddress(transportAddress))
            except Exception:
                raise error.CarrierError(';'.join(traceback.format_exception(*sys.exc_info())))

    def normalizeAddress(self, transportAddress):
        if not isinstance(transportAddress, self.addressType):
            transportAddress = self.addressType(transportAddress)
        return transportAddress
