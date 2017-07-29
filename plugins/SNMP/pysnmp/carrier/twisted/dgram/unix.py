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

domainName = snmpLocalDomain = (1, 3, 6, 1, 2, 1, 100, 1, 13)

class UnixTransportAddress(str, AbstractTransportAddress):
    pass

class UnixTwistedTransport(DgramTwistedTransport):
    addressType = UnixTransportAddress
    _lport = None

    # AbstractTwistedTransport API

    def openClientMode(self, iface=''):
        try:
            self._lport = reactor.connectUNIXDatagram(iface, self)
        except Exception:
            raise error.CarrierError(sys.exc_info()[1])
        return self

    def openServerMode(self, iface):
        try:
            self._lport = reactor.listenUNIXDatagram(iface, self)
        except Exception:
            raise error.CarrierError(sys.exc_info()[1])

        return self

    def closeTransport(self):
        if self._lport is not None:
            d = self._lport.stopListening()
            if d:
                d.addCallback(lambda x: None)
        DgramTwistedTransport.closeTransport(self)

UnixTransport = UnixTwistedTransport
