#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import socket

symbols = {
    'IP_PKTINFO': 8,
    'IP_TRANSPARENT': 19,
    'SOL_IPV6': 41,
    'IPV6_RECVPKTINFO': 49,
    'IPV6_PKTINFO': 50
}

for symbol in symbols:
    if not hasattr(socket, symbol):
        setattr(socket, symbol, symbols[symbol])
