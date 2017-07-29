#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor
from pysnmp.carrier.twisted.base import AbstractTwistedTransport
from pysnmp.carrier import error
from pysnmp import debug


class DgramTwistedTransport(DatagramProtocol, AbstractTwistedTransport):
    """Base Twisted datagram Transport, to be used with TwistedDispatcher"""

    # Twisted Datagram API

    def datagramReceived(self, datagram, transportAddress):
        if self._cbFun is None:
            raise error.CarrierError('Unable to call cbFun')
        else:
            # Callback fun is called through callLater() in attempt
            # to make Twisted timed calls work under high load.
            reactor.callLater(0, self._cbFun, self, transportAddress, datagram)

    def startProtocol(self):
        debug.logger & debug.flagIO and debug.logger('startProtocol: invoked')
        while self._writeQ:
            outgoingMessage, transportAddress = self._writeQ.pop(0)
            debug.logger & debug.flagIO and debug.logger('startProtocol: transportAddress %r outgoingMessage %s' % (transportAddress, debug.hexdump(outgoingMessage)))
            try:
                self.transport.write(outgoingMessage, transportAddress)
            except Exception:
                raise error.CarrierError('Twisted exception: %s' % (sys.exc_info()[1],))

    def stopProtocol(self):
        debug.logger & debug.flagIO and debug.logger('stopProtocol: invoked')

    def sendMessage(self, outgoingMessage, transportAddress):
        debug.logger & debug.flagIO and debug.logger('startProtocol: %s transportAddress %r outgoingMessage %s' % ((self.transport is None and "queuing" or "sending"), transportAddress, debug.hexdump(outgoingMessage)))
        if self.transport is None:
            self._writeQ.append((outgoingMessage, transportAddress))
        else:
            try:
                self.transport.write(outgoingMessage, transportAddress)
            except Exception:
                raise error.CarrierError('Twisted exception: %s' % (sys.exc_info()[1],))
