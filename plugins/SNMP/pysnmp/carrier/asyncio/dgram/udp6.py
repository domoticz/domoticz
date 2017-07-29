#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import socket
from pysnmp.carrier.base import AbstractTransportAddress
from pysnmp.carrier.asyncio.dgram.base import DgramAsyncioProtocol


domainName = snmpUDP6Domain = (1, 3, 6, 1, 2, 1, 100, 1, 2)


class Udp6TransportAddress(tuple, AbstractTransportAddress):
    pass


class Udp6AsyncioTransport(DgramAsyncioProtocol):
    sockFamily = socket.has_ipv6 and socket.AF_INET6 or None
    addressType = Udp6TransportAddress

    def normalizeAddress(self, transportAddress):
        if '%' in transportAddress[0]:  # strip zone ID
            return self.addressType((transportAddress[0].split('%')[0],
                                     transportAddress[1],
                                     0,  # flowinfo
                                     0))  # scopeid
        else:
            return self.addressType((transportAddress[0],
                                     transportAddress[1], 0, 0))


Udp6Transport = Udp6AsyncioTransport
