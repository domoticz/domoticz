#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.carrier import sockfix
from pysnmp.carrier.base import AbstractTransportAddress
from pysnmp.carrier.asyncore.dgram.base import DgramSocketTransport
import socket

domainName = snmpUDP6Domain = (1, 3, 6, 1, 2, 1, 100, 1, 2)


class Udp6TransportAddress(tuple, AbstractTransportAddress):
    pass


class Udp6SocketTransport(DgramSocketTransport):
    sockFamily = socket.has_ipv6 and socket.AF_INET6 or None
    addressType = Udp6TransportAddress

    def normalizeAddress(self, transportAddress):
        if '%' in transportAddress[0]:  # strip zone ID
            ta = self.addressType((transportAddress[0].split('%')[0],
                                   transportAddress[1],
                                   0,  # flowinfo
                                   0))  # scopeid
        else:
            ta = self.addressType((transportAddress[0],
                                   transportAddress[1], 0, 0))

        if (isinstance(transportAddress, self.addressType) and
                transportAddress.getLocalAddress()):
            return ta.setLocalAddress(transportAddress.getLocalAddress())
        else:
            return ta.setLocalAddress(self.getLocalAddress())


Udp6Transport = Udp6SocketTransport
