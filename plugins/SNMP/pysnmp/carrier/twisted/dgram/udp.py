#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from twisted.internet import reactor
from pysnmp.carrier.base import AbstractTransportAddress
from pysnmp.carrier.twisted.dgram.base import DgramTwistedTransport
from pysnmp.carrier import error

domainName = snmpUDPDomain = (1, 3, 6, 1, 6, 1, 1)


class UdpTransportAddress(tuple, AbstractTransportAddress):
    pass


class UdpTwistedTransport(DgramTwistedTransport):
    addressType = UdpTransportAddress
    _lport = None

    # AbstractTwistedTransport API

    def openClientMode(self, iface=None):
        if iface is None:
            iface = ('', 0)
        try:
            self._lport = reactor.listenUDP(iface[1], self, iface[0])
        except Exception:
            raise error.CarrierError(sys.exc_info()[1])
        return self

    def openServerMode(self, iface):
        try:
            self._lport = reactor.listenUDP(iface[1], self, iface[0])
        except Exception:
            raise error.CarrierError(sys.exc_info()[1])
        return self

    def closeTransport(self):
        if self._lport is not None:
            d = self._lport.stopListening()
            if d:
                d.addCallback(lambda x: None)
            DgramTwistedTransport.closeTransport(self)


UdpTransport = UdpTwistedTransport
