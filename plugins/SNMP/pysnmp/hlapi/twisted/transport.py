#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import socket, sys
from pysnmp.carrier.twisted.dgram import udp
from pysnmp.hlapi.transport import AbstractTransportTarget
from pysnmp.error import PySnmpError

__all__ = ['UdpTransportTarget']

class UdpTransportTarget(AbstractTransportTarget):
    """Creates UDP/IPv4 configuration entry and initialize socket API if needed.

    This object can be used for adding new entries to Local Configuration
    Datastore (LCD) managed by :py:class:`~pysnmp.hlapi.SnmpEngine`
    class instance.

    See :RFC:`1906#section-3` for more information on the UDP transport mapping.

    Parameters
    ----------
    transportAddr : tuple
        Indicates remote address in Python :py:mod:`socket` module format
        which is a tuple of FQDN, port where FQDN is a string representing
        either hostname or IPv4 address in quad-dotted form, port is an
        integer.
    timeout : int
        Response timeout in seconds.
    retries : int
        Maximum number of request retries, 0 retries means just a single
        request.
    tagList : str
        Arbitrary string that contains a list of tag values which are used
        to select target addresses for a particular operation
        (:RFC:`3413#section-4.1.4`).

    Examples
    --------
    >>> from pysnmp.hlapi.twisted import UdpTransportTarget
    >>> UdpTransportTarget(('demo.snmplabs.com', 161))
    UdpTransportTarget(('195.218.195.228', 161), timeout=1, retries=5, tagList='')
    >>>

    """
    transportDomain = udp.domainName
    protoTransport = udp.UdpTwistedTransport
    def _resolveAddr(self, transportAddr):
        try:
            return socket.getaddrinfo(transportAddr[0],
                                      transportAddr[1],
                                      socket.AF_INET,
                                      socket.SOCK_DGRAM,
                                      socket.IPPROTO_UDP)[0][4][:2]
        except socket.gaierror:
            raise PySnmpError('Bad IPv4/UDP transport address %s: %s' % ('@'.join([str(x) for x in transportAddr]), sys.exc_info()[1]))
