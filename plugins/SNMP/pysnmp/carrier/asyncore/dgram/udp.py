#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from socket import AF_INET
from pysnmp.carrier.base import AbstractTransportAddress
from pysnmp.carrier.asyncore.dgram.base import DgramSocketTransport

domainName = snmpUDPDomain = (1, 3, 6, 1, 6, 1, 1)


class UdpTransportAddress(tuple, AbstractTransportAddress):
    pass


class UdpSocketTransport(DgramSocketTransport):
    sockFamily = AF_INET
    addressType = UdpTransportAddress


UdpTransport = UdpSocketTransport
