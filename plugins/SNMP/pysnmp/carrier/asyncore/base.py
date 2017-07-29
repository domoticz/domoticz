#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import socket
import sys
import asyncore
from pysnmp.carrier import error
from pysnmp.carrier.base import AbstractTransport
from pysnmp.carrier.asyncore.dispatch import AsyncoreDispatcher
from pysnmp import debug


class AbstractSocketTransport(asyncore.dispatcher, AbstractTransport):
    protoTransportDispatcher = AsyncoreDispatcher
    sockFamily = sockType = None
    retryCount = 0
    retryInterval = 0
    bufferSize = 131070

    # noinspection PyUnusedLocal
    def __init__(self, sock=None, sockMap=None):
        asyncore.dispatcher.__init__(self)
        if sock is None:
            if self.sockFamily is None:
                raise error.CarrierError(
                    'Address family %s not supported' % self.__class__.__name__
                )
            if self.sockType is None:
                raise error.CarrierError(
                    'Socket type %s not supported' % self.__class__.__name__
                )
            try:
                sock = socket.socket(self.sockFamily, self.sockType)
            except socket.error:
                raise error.CarrierError('socket() failed: %s' % sys.exc_info()[1])

            try:
                for b in socket.SO_RCVBUF, socket.SO_SNDBUF:
                    bsize = sock.getsockopt(socket.SOL_SOCKET, b)
                    if bsize < self.bufferSize:
                        sock.setsockopt(socket.SOL_SOCKET, b, self.bufferSize)
                        debug.logger & debug.flagIO and debug.logger('%s: socket %d buffer size increased from %d to %d for buffer %d' % (self.__class__.__name__, sock.fileno(), bsize, self.bufferSize, b))
            except Exception:
                debug.logger & debug.flagIO and debug.logger('%s: socket buffer size option mangling failure for buffer: %s' % (self.__class__.__name__, sys.exc_info()[1]))

        # The socket map is managed by the AsyncoreDispatcher on
        # which this transport is registered. Here we just prepare
        # socket and postpone transport registration at dispatcher
        # till AsyncoreDispatcher invokes registerSocket()

        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setblocking(0)
        self.set_socket(sock)

    def __hash__(self):
        return hash(self.socket)

    # The following two methods are part of base class so here we overwrite
    # them to separate socket management from dispatcher registration tasks.
    # These two are just for dispatcher registration.
    def add_channel(self, map=None):
        if map is not None:
            map[self._fileno] = self
            self.connected = True

    def del_channel(self, map=None):
        if map is not None and self._fileno in map:
            del map[self._fileno]
            self.connected = False

    def registerSocket(self, sockMap=None):
        self.add_channel(sockMap)

    def unregisterSocket(self, sockMap=None):
        self.del_channel(sockMap)

    def closeTransport(self):
        AbstractTransport.closeTransport(self)
        self.close()

    # asyncore API
    def handle_close(self):
        raise error.CarrierError('Transport unexpectedly closed')

    def handle_error(self):
        raise
