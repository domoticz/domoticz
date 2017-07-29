#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# PySNMP MIB module TRANSPORT-ADDRESS-MIB (http://pysnmp.sf.net)
# ASN.1 source http://mibs.snmplabs.com:80/asn1/TRANSPORT-ADDRESS-MIB
# Produced by pysmi-0.1.3 at Tue Apr 18 01:08:18 2017
# On host grommit.local platform Darwin version 16.4.0 by user ilya
# Using Python version 3.4.2 (v3.4.2:ab2c023a9432, Oct  5 2014, 20:42:22)
#

from pyasn1.compat.octets import int2oct, oct2int
from pysnmp import error
from pysnmp.carrier import sockfix
import socket

has_ipv6 = socket.has_ipv6

if hasattr(socket, 'inet_ntop') and hasattr(socket, 'inet_pton'):
    inet_ntop = socket.inet_ntop
    inet_pton = socket.inet_pton
else:
    import sys

    if sys.platform != "win32":
        from socket import inet_ntoa, inet_aton

        inet_ntop = lambda x, y: inet_ntoa(y)
        inet_pton = lambda x, y: inet_aton(y)
        has_ipv6 = False
    elif has_ipv6:
        import struct  # The case of old Python at old Windows


        def inet_pton(address_family, ip_string):
            if address_family == socket.AF_INET:
                return inet_aton(ip_string)
            elif address_family != socket.AF_INET6:
                raise socket.error(
                    'Unknown address family %s' % (address_family,)
                )

            groups = ip_string.split(":")
            spaces = groups.count('')

            if '.' in groups[-1]:
                groups[-1:] = ["%x" % x for x in struct.unpack("!HH", inet_aton(groups[-1]))]

            if spaces == 1:
                idx = groups.index('')
                groups[idx:idx + 1] = ['0'] * (8 - len(groups) + 1)
            elif spaces == 2:
                zeros = ['0'] * (8 - len(groups) + 2)
                if ip_string.startswith('::'):
                    groups[:2] = zeros
                elif ip_string.endswith('::'):
                    groups[-2:] = zeros
                else:
                    raise socket.error(
                        'Invalid IPv6 address: "%s"' % (ip_string,)
                    )
            elif spaces == 3:
                if ip_string != '::':
                    raise socket.error(
                        'Invalid IPv6 address: "%s"' % (ip_string,)
                    )
                return '\x00' * 16
            elif spaces > 3:
                raise socket.error(
                    'Invalid IPv6 address: "%s"' % (ip_string,)
                )

            groups = [t for t in [int(t, 16) for t in groups] if t & 0xFFFF == t]

            if len(groups) != 8:
                raise socket.error(
                    'Invalid IPv6 address: "%s"' % (ip_string,)
                )

            return struct.pack('!8H', *groups)


        def inet_ntop(address_family, packed_ip):
            if address_family == socket.AF_INET:
                return inet_ntop(packed_ip)
            elif address_family != socket.AF_INET6:
                raise socket.error(
                    'Unknown address family %s' % (address_family,)
                )

            if len(packed_ip) != 16:
                raise socket.error(
                    'incorrect address length: %s' % len(packed_ip)
                )

            groups = list(struct.unpack('!8H', packed_ip))

            cur_base = best_base = cur_len = best_len = -1

            for idx in range(8):
                if groups[idx]:
                    if cur_base != -1:
                        if best_base == -1 or cur_len > best_len:
                            best_base, best_len = cur_base, cur_len
                        cur_base = -1
                else:
                    if cur_base == -1:
                        cur_base, cur_len = idx, 1
                    else:
                        cur_len += 1

            if cur_base != -1:
                if best_base == -1 or cur_len > best_len:
                    best_base, best_len = cur_base, cur_len

            if best_base != -1 and best_len > 1:
                groups[best_base:best_base + best_len] = [':']

            if groups[0] == ':':
                groups.insert(0, ':')
            if groups[-1] == ':':
                groups.append(':')

            f = lambda x: x != ':' and '%x' % x or ''

            return ':'.join([f(x) for x in groups])

Integer, OctetString, ObjectIdentifier = mibBuilder.importSymbols("ASN1", "Integer", "OctetString", "ObjectIdentifier")
NamedValues, = mibBuilder.importSymbols("ASN1-ENUMERATION", "NamedValues")
ConstraintsIntersection, ConstraintsUnion, ValueRangeConstraint, ValueSizeConstraint, SingleValueConstraint = mibBuilder.importSymbols("ASN1-REFINEMENT", "ConstraintsIntersection", "ConstraintsUnion", "ValueRangeConstraint", "ValueSizeConstraint", "SingleValueConstraint")
NotificationGroup, ModuleCompliance = mibBuilder.importSymbols("SNMPv2-CONF", "NotificationGroup", "ModuleCompliance")
Integer32, Unsigned32, MibIdentifier, Counter32, ModuleIdentity, MibScalar, MibTable, MibTableRow, MibTableColumn, NotificationType, Bits, TimeTicks, ObjectIdentity, Counter64, Gauge32, IpAddress, iso, mib_2 = mibBuilder.importSymbols("SNMPv2-SMI", "Integer32", "Unsigned32", "MibIdentifier", "Counter32", "ModuleIdentity", "MibScalar", "MibTable", "MibTableRow", "MibTableColumn", "NotificationType", "Bits", "TimeTicks", "ObjectIdentity", "Counter64", "Gauge32", "IpAddress", "iso", "mib-2")
DisplayString, TextualConvention = mibBuilder.importSymbols("SNMPv2-TC", "DisplayString", "TextualConvention")
transportAddressMIB = ModuleIdentity((1, 3, 6, 1, 2, 1, 100))
if mibBuilder.loadTexts: transportAddressMIB.setRevisions(('2002-11-01 00:00',))
if mibBuilder.loadTexts: transportAddressMIB.setLastUpdated('200211010000Z')
if mibBuilder.loadTexts: transportAddressMIB.setOrganization('IETF Operations and Management Area')
if mibBuilder.loadTexts: transportAddressMIB.setContactInfo('Juergen Schoenwaelder (Editor) TU Braunschweig Bueltenweg 74/75 38106 Braunschweig, Germany Phone: +49 531 391-3289 EMail: schoenw@ibr.cs.tu-bs.de Send comments to <mibs@ops.ietf.org>.')
if mibBuilder.loadTexts: transportAddressMIB.setDescription('This MIB module provides commonly used transport address definitions. Copyright (C) The Internet Society (2002). This version of this MIB module is part of RFC 3419; see the RFC itself for full legal notices.')
transportDomains = MibIdentifier((1, 3, 6, 1, 2, 1, 100, 1))
transportDomainUdpIpv4 = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 1))
if mibBuilder.loadTexts: transportDomainUdpIpv4.setStatus('current')
if mibBuilder.loadTexts: transportDomainUdpIpv4.setDescription('The UDP over IPv4 transport domain. The corresponding transport address is of type TransportAddressIPv4 for global IPv4 addresses.')
transportDomainUdpIpv6 = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 2))
if mibBuilder.loadTexts: transportDomainUdpIpv6.setStatus('current')
if mibBuilder.loadTexts: transportDomainUdpIpv6.setDescription('The UDP over IPv6 transport domain. The corresponding transport address is of type TransportAddressIPv6 for global IPv6 addresses.')
transportDomainUdpIpv4z = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 3))
if mibBuilder.loadTexts: transportDomainUdpIpv4z.setStatus('current')
if mibBuilder.loadTexts: transportDomainUdpIpv4z.setDescription('The UDP over IPv4 transport domain. The corresponding transport address is of type TransportAddressIPv4z for scoped IPv4 addresses with a zone index.')
transportDomainUdpIpv6z = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 4))
if mibBuilder.loadTexts: transportDomainUdpIpv6z.setStatus('current')
if mibBuilder.loadTexts: transportDomainUdpIpv6z.setDescription('The UDP over IPv6 transport domain. The corresponding transport address is of type TransportAddressIPv6z for scoped IPv6 addresses with a zone index.')
transportDomainTcpIpv4 = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 5))
if mibBuilder.loadTexts: transportDomainTcpIpv4.setStatus('current')
if mibBuilder.loadTexts: transportDomainTcpIpv4.setDescription('The TCP over IPv4 transport domain. The corresponding transport address is of type TransportAddressIPv4 for global IPv4 addresses.')
transportDomainTcpIpv6 = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 6))
if mibBuilder.loadTexts: transportDomainTcpIpv6.setStatus('current')
if mibBuilder.loadTexts: transportDomainTcpIpv6.setDescription('The TCP over IPv6 transport domain. The corresponding transport address is of type TransportAddressIPv6 for global IPv6 addresses.')
transportDomainTcpIpv4z = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 7))
if mibBuilder.loadTexts: transportDomainTcpIpv4z.setStatus('current')
if mibBuilder.loadTexts: transportDomainTcpIpv4z.setDescription('The TCP over IPv4 transport domain. The corresponding transport address is of type TransportAddressIPv4z for scoped IPv4 addresses with a zone index.')
transportDomainTcpIpv6z = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 8))
if mibBuilder.loadTexts: transportDomainTcpIpv6z.setStatus('current')
if mibBuilder.loadTexts: transportDomainTcpIpv6z.setDescription('The TCP over IPv6 transport domain. The corresponding transport address is of type TransportAddressIPv6z for scoped IPv6 addresses with a zone index.')
transportDomainSctpIpv4 = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 9))
if mibBuilder.loadTexts: transportDomainSctpIpv4.setStatus('current')
if mibBuilder.loadTexts: transportDomainSctpIpv4.setDescription('The SCTP over IPv4 transport domain. The corresponding transport address is of type TransportAddressIPv4 for global IPv4 addresses. This transport domain usually represents the primary address on multihomed SCTP endpoints.')
transportDomainSctpIpv6 = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 10))
if mibBuilder.loadTexts: transportDomainSctpIpv6.setStatus('current')
if mibBuilder.loadTexts: transportDomainSctpIpv6.setDescription('The SCTP over IPv6 transport domain. The corresponding transport address is of type TransportAddressIPv6 for global IPv6 addresses. This transport domain usually represents the primary address on multihomed SCTP endpoints.')
transportDomainSctpIpv4z = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 11))
if mibBuilder.loadTexts: transportDomainSctpIpv4z.setStatus('current')
if mibBuilder.loadTexts: transportDomainSctpIpv4z.setDescription('The SCTP over IPv4 transport domain. The corresponding transport address is of type TransportAddressIPv4z for scoped IPv4 addresses with a zone index. This transport domain usually represents the primary address on multihomed SCTP endpoints.')
transportDomainSctpIpv6z = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 12))
if mibBuilder.loadTexts: transportDomainSctpIpv6z.setStatus('current')
if mibBuilder.loadTexts: transportDomainSctpIpv6z.setDescription('The SCTP over IPv6 transport domain. The corresponding transport address is of type TransportAddressIPv6z for scoped IPv6 addresses with a zone index. This transport domain usually represents the primary address on multihomed SCTP endpoints.')
transportDomainLocal = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 13))
if mibBuilder.loadTexts: transportDomainLocal.setStatus('current')
if mibBuilder.loadTexts: transportDomainLocal.setDescription('The Posix Local IPC transport domain. The corresponding transport address is of type TransportAddressLocal. The Posix Local IPC transport domain incorporates the well-known UNIX domain sockets.')
transportDomainUdpDns = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 14))
if mibBuilder.loadTexts: transportDomainUdpDns.setStatus('current')
if mibBuilder.loadTexts: transportDomainUdpDns.setDescription('The UDP transport domain using fully qualified domain names. The corresponding transport address is of type TransportAddressDns.')
transportDomainTcpDns = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 15))
if mibBuilder.loadTexts: transportDomainTcpDns.setStatus('current')
if mibBuilder.loadTexts: transportDomainTcpDns.setDescription('The TCP transport domain using fully qualified domain names. The corresponding transport address is of type TransportAddressDns.')
transportDomainSctpDns = ObjectIdentity((1, 3, 6, 1, 2, 1, 100, 1, 16))
if mibBuilder.loadTexts: transportDomainSctpDns.setStatus('current')
if mibBuilder.loadTexts: transportDomainSctpDns.setDescription('The SCTP transport domain using fully qualified domain names. The corresponding transport address is of type TransportAddressDns.')

class TransportDomain(TextualConvention, ObjectIdentifier):
    description = 'A value that represents a transport domain. Some possible values, such as transportDomainUdpIpv4, are defined in this module. Other possible values can be defined in other MIB modules.'
    status = 'current'

class TransportAddressType(TextualConvention, Integer32):
    description = 'A value that represents a transport domain. This is the enumerated version of the transport domain registrations in this MIB module. The enumerated values have the following meaning: unknown(0) unknown transport address type udpIpv4(1) transportDomainUdpIpv4 udpIpv6(2) transportDomainUdpIpv6 udpIpv4z(3) transportDomainUdpIpv4z udpIpv6z(4) transportDomainUdpIpv6z tcpIpv4(5) transportDomainTcpIpv4 tcpIpv6(6) transportDomainTcpIpv6 tcpIpv4z(7) transportDomainTcpIpv4z tcpIpv6z(8) transportDomainTcpIpv6z sctpIpv4(9) transportDomainSctpIpv4 sctpIpv6(10) transportDomainSctpIpv6 sctpIpv4z(11) transportDomainSctpIpv4z sctpIpv6z(12) transportDomainSctpIpv6z local(13) transportDomainLocal udpDns(14) transportDomainUdpDns tcpDns(15) transportDomainTcpDns sctpDns(16) transportDomainSctpDns This textual convention can be used to represent transport domains in situations where a syntax of TransportDomain is unwieldy (for example, when used as an index). The usage of this textual convention implies that additional transport domains can only be supported by updating this MIB module. This extensibility restriction does not apply for the TransportDomain textual convention which allows MIB authors to define additional transport domains independently in other MIB modules.'
    status = 'current'
    subtypeSpec = Integer32.subtypeSpec + ConstraintsUnion(SingleValueConstraint(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
    namedValues = NamedValues(("unknown", 0), ("udpIpv4", 1), ("udpIpv6", 2), ("udpIpv4z", 3), ("udpIpv6z", 4), ("tcpIpv4", 5), ("tcpIpv6", 6), ("tcpIpv4z", 7), ("tcpIpv6z", 8), ("sctpIpv4", 9), ("sctpIpv6", 10), ("sctpIpv4z", 11), ("sctpIpv6z", 12), ("local", 13), ("udpDns", 14), ("tcpDns", 15), ("sctpDns", 16))

class TransportAddress(TextualConvention, OctetString):
    description = "Denotes a generic transport address. A TransportAddress value is always interpreted within the context of a TransportAddressType or TransportDomain value. Every usage of the TransportAddress textual convention MUST specify the TransportAddressType or TransportDomain object which provides the context. Furthermore, MIB authors SHOULD define a separate TransportAddressType or TransportDomain object for each TransportAddress object. It is suggested that the TransportAddressType or TransportDomain is logically registered before the object(s) which use the TransportAddress textual convention if they appear in the same logical row. The value of a TransportAddress object must always be consistent with the value of the associated TransportAddressType or TransportDomain object. Attempts to set a TransportAddress object to a value which is inconsistent with the associated TransportAddressType or TransportDomain must fail with an inconsistentValue error. When this textual convention is used as a syntax of an index object, there may be issues with the limit of 128 sub-identifiers specified in SMIv2, STD 58. In this case, the OBJECT-TYPE declaration MUST include a 'SIZE' clause to limit the number of potential instance sub-identifiers."
    status = 'current'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(0, 255)

class TransportAddressIPv4(TextualConvention, OctetString):
    description = 'Represents a transport address consisting of an IPv4 address and a port number (as used for example by UDP, TCP and SCTP): octets contents encoding 1-4 IPv4 address network-byte order 5-6 port number network-byte order This textual convention SHOULD NOT be used directly in object definitions since it restricts addresses to a specific format. However, if it is used, it MAY be used either on its own or in conjunction with TransportAddressType or TransportDomain as a pair.'
    status = 'current'
    displayHint = '1d.1d.1d.1d:2d'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(6, 6)
    fixedLength = 6

    def prettyIn(self, value):
        if isinstance(value, tuple):
            # Wild hack -- need to implement TextualConvention.prettyIn
            value = inet_pton(socket.AF_INET, value[0]) + int2oct((value[1] >> 8) & 0xff) + int2oct(value[1] & 0xff)
        return OctetString.prettyIn(self, value)

    # Socket address syntax coercion
    def __asSocketAddress(self):
        if not hasattr(self, '__tuple_value'):
            v = self.asOctets()
            self.__tuple_value = (
                inet_ntop(socket.AF_INET, v[:4]),
                oct2int(v[4]) << 8 | oct2int(v[5]),
            )
        return self.__tuple_value

    def __iter__(self):
        return iter(self.__asSocketAddress())

    def __getitem__(self, item):
        return self.__asSocketAddress()[item]


class TransportAddressIPv6(TextualConvention, OctetString):
    description = 'Represents a transport address consisting of an IPv6 address and a port number (as used for example by UDP, TCP and SCTP): octets contents encoding 1-16 IPv6 address network-byte order 17-18 port number network-byte order This textual convention SHOULD NOT be used directly in object definitions since it restricts addresses to a specific format. However, if it is used, it MAY be used either on its own or in conjunction with TransportAddressType or TransportDomain as a pair.'
    status = 'current'
    displayHint = '0a[2x:2x:2x:2x:2x:2x:2x:2x]0a:2d'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(18, 18)
    fixedLength = 18

    def prettyIn(self, value):
        if not has_ipv6:
            raise error.PySnmpError('IPv6 not supported by platform')
        if isinstance(value, tuple):
            value = inet_pton(socket.AF_INET6, value[0]) + int2oct((value[1] >> 8) & 0xff) + int2oct(value[1] & 0xff)
        return OctetString.prettyIn(self, value)

    # Socket address syntax coercion
    def __asSocketAddress(self):
        if not hasattr(self, '__tuple_value'):
            if not has_ipv6:
                raise error.PySnmpError('IPv6 not supported by platform')
            v = self.asOctets()
            self.__tuple_value = (
                inet_ntop(socket.AF_INET6, v[:16]),
                oct2int(v[16]) << 8 | oct2int(v[17]),
                0,  # flowinfo
                0)  # scopeid
        return self.__tuple_value

    def __iter__(self):
        return iter(self.__asSocketAddress())

    def __getitem__(self, item):
        return self.__asSocketAddress()[item]

class TransportAddressIPv4z(TextualConvention, OctetString):
    description = 'Represents a transport address consisting of an IPv4 address, a zone index and a port number (as used for example by UDP, TCP and SCTP): octets contents encoding 1-4 IPv4 address network-byte order 5-8 zone index network-byte order 9-10 port number network-byte order This textual convention SHOULD NOT be used directly in object definitions since it restricts addresses to a specific format. However, if it is used, it MAY be used either on its own or in conjunction with TransportAddressType or TransportDomain as a pair.'
    status = 'current'
    displayHint = '1d.1d.1d.1d%4d:2d'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(10, 10)
    fixedLength = 10

class TransportAddressIPv6z(TextualConvention, OctetString):
    description = 'Represents a transport address consisting of an IPv6 address, a zone index and a port number (as used for example by UDP, TCP and SCTP): octets contents encoding 1-16 IPv6 address network-byte order 17-20 zone index network-byte order 21-22 port number network-byte order This textual convention SHOULD NOT be used directly in object definitions since it restricts addresses to a specific format. However, if it is used, it MAY be used either on its own or in conjunction with TransportAddressType or TransportDomain as a pair.'
    status = 'current'
    displayHint = '0a[2x:2x:2x:2x:2x:2x:2x:2x%4d]0a:2d'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(22, 22)
    fixedLength = 22

class TransportAddressLocal(TextualConvention, OctetString):
    reference = 'Protocol Independent Interfaces (IEEE POSIX 1003.1g)'
    description = "Represents a POSIX Local IPC transport address: octets contents encoding all POSIX Local IPC address string The Posix Local IPC transport domain subsumes UNIX domain sockets. This textual convention SHOULD NOT be used directly in object definitions since it restricts addresses to a specific format. However, if it is used, it MAY be used either on its own or in conjunction with TransportAddressType or TransportDomain as a pair. When this textual convention is used as a syntax of an index object, there may be issues with the limit of 128 sub-identifiers specified in SMIv2, STD 58. In this case, the OBJECT-TYPE declaration MUST include a 'SIZE' clause to limit the number of potential instance sub-identifiers."
    status = 'current'
    displayHint = '1a'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(1, 255)

class TransportAddressDns(TextualConvention, OctetString):
    description = "Represents a DNS domain name followed by a colon ':' (ASCII character 0x3A) and a port number in ASCII. The name SHOULD be fully qualified whenever possible. Values of this textual convention are not directly useable as transport-layer addressing information, and require runtime resolution. As such, applications that write them must be prepared for handling errors if such values are not supported, or cannot be resolved (if resolution occurs at the time of the management operation). The DESCRIPTION clause of TransportAddress objects that may have TransportAddressDns values must fully describe how (and when) such names are to be resolved to IP addresses and vice versa. This textual convention SHOULD NOT be used directly in object definitions since it restricts addresses to a specific format. However, if it is used, it MAY be used either on its own or in conjunction with TransportAddressType or TransportDomain as a pair. When this textual convention is used as a syntax of an index object, there may be issues with the limit of 128 sub-identifiers specified in SMIv2, STD 58. In this case, the OBJECT-TYPE declaration MUST include a 'SIZE' clause to limit the number of potential instance sub-identifiers."
    status = 'current'
    displayHint = '1a'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(1, 255)

mibBuilder.exportSymbols("TRANSPORT-ADDRESS-MIB", transportDomainUdpIpv6=transportDomainUdpIpv6, TransportDomain=TransportDomain, transportDomainSctpDns=transportDomainSctpDns, PYSNMP_MODULE_ID=transportAddressMIB, TransportAddressType=TransportAddressType, transportDomainTcpIpv4=transportDomainTcpIpv4, TransportAddressIPv6z=TransportAddressIPv6z, transportDomainLocal=transportDomainLocal, TransportAddressLocal=TransportAddressLocal, transportDomainUdpIpv4=transportDomainUdpIpv4, transportDomainUdpIpv4z=transportDomainUdpIpv4z, transportDomains=transportDomains, transportDomainSctpIpv4=transportDomainSctpIpv4, TransportAddressIPv6=TransportAddressIPv6, TransportAddressDns=TransportAddressDns, transportDomainSctpIpv6z=transportDomainSctpIpv6z, transportAddressMIB=transportAddressMIB, transportDomainTcpIpv6z=transportDomainTcpIpv6z, transportDomainSctpIpv6=transportDomainSctpIpv6, transportDomainUdpDns=transportDomainUdpDns, transportDomainTcpIpv6=transportDomainTcpIpv6, transportDomainUdpIpv6z=transportDomainUdpIpv6z, TransportAddressIPv4=TransportAddressIPv4, transportDomainSctpIpv4z=transportDomainSctpIpv4z, transportDomainTcpIpv4z=transportDomainTcpIpv4z, TransportAddress=TransportAddress, transportDomainTcpDns=transportDomainTcpDns, TransportAddressIPv4z=TransportAddressIPv4z)
