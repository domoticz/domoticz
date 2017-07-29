#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# PySNMP MIB module SNMPv2-TM (http://pysnmp.sf.net)
# ASN.1 source http://mibs.snmplabs.com:80/asn1/SNMPv2-TM
# Produced by pysmi-0.1.3 at Tue Apr 18 01:01:19 2017
# On host grommit.local platform Darwin version 16.4.0 by user ilya
# Using Python version 3.4.2 (v3.4.2:ab2c023a9432, Oct  5 2014, 20:42:22)
#
try:
    from socket import inet_ntop, inet_pton, AF_INET
except ImportError:
    from socket import inet_ntoa, inet_aton, AF_INET

    inet_ntop = lambda x, y: inet_ntoa(y)
    inet_pton = lambda x, y: inet_aton(y)

from pyasn1.compat.octets import int2oct, oct2int

OctetString, = mibBuilder.importSymbols('ASN1', 'OctetString')
ConstraintsIntersection, ConstraintsUnion, SingleValueConstraint, ValueRangeConstraint, ValueSizeConstraint = mibBuilder.importSymbols("ASN1-REFINEMENT", "ConstraintsIntersection", "ConstraintsUnion", "SingleValueConstraint", "ValueRangeConstraint", "ValueSizeConstraint")
ModuleIdentity, MibIdentifier, ObjectIdentity, snmpModules, snmpDomains, snmpProxys = mibBuilder.importSymbols('SNMPv2-SMI', 'ModuleIdentity', 'MibIdentifier', 'ObjectIdentity', 'snmpModules', 'snmpDomains', 'snmpProxys')
TextualConvention, = mibBuilder.importSymbols('SNMPv2-TC', 'TextualConvention')

snmpv2tm = ModuleIdentity((1, 3, 6, 1, 6, 3, 19))
if mibBuilder.loadTexts: snmpv2tm.setRevisions(('2000-08-09 19:58', '1996-01-01 00:00', '1993-04-01 00:00',))
if mibBuilder.loadTexts: snmpv2tm.setLastUpdated('200008091958Z')
if mibBuilder.loadTexts: snmpv2tm.setOrganization('IETF SNMPv3 Working Group')
if mibBuilder.loadTexts: snmpv2tm.setContactInfo('WG-EMail: snmpv3@tis.com Subscribe: majordomo@tis.com In message body: subscribe snmpv3 Chair: Russ Mundy TIS Labs at Network Associates postal: 3060 Washington Rd Glenwood MD 21738 USA EMail: mundy@tislabs.com phone: +1 301 854-6889 Editor: Randy Presuhn BMC Software, Inc. postal: 2141 North First Street San Jose, CA 95131 USA EMail: randy-presuhn@bmc.com phone: +1 408 546-1006')
if mibBuilder.loadTexts: snmpv2tm.setDescription('The MIB module for SNMP transport mappings.')
snmpUDPDomain = ObjectIdentity((1, 3, 6, 1, 6, 1, 1))
if mibBuilder.loadTexts: snmpUDPDomain.setStatus('current')
if mibBuilder.loadTexts: snmpUDPDomain.setDescription('The SNMP over UDP over IPv4 transport domain. The corresponding transport address is of type SnmpUDPAddress.')


class SnmpUDPAddress(TextualConvention, OctetString):
    description = 'Represents a UDP over IPv4 address: octets contents encoding 1-4 IP-address network-byte order 5-6 UDP-port network-byte order '
    status = 'current'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(6, 6)
    displayHint = "1d.1d.1d.1d/2d"
    fixedLength = 6

    def prettyIn(self, value):
        if isinstance(value, tuple):
            # Wild hack -- need to implement TextualConvention.prettyIn
            value = inet_pton(AF_INET, value[0]) + int2oct((value[1] >> 8) & 0xff) + int2oct(value[1] & 0xff)
        return OctetString.prettyIn(self, value)

    # Socket address syntax coercion
    def __asSocketAddress(self):
        if not hasattr(self, '__tuple_value'):
            v = self.asOctets()
            self.__tuple_value = (
                inet_ntop(AF_INET, v[:4]),
                oct2int(v[4]) << 8 | oct2int(v[5])
            )
        return self.__tuple_value

    def __iter__(self):
        return iter(self.__asSocketAddress())

    def __getitem__(self, item):
        return self.__asSocketAddress()[item]


snmpCLNSDomain = ObjectIdentity((1, 3, 6, 1, 6, 1, 2))
if mibBuilder.loadTexts: snmpCLNSDomain.setStatus('current')
if mibBuilder.loadTexts: snmpCLNSDomain.setDescription('The SNMP over CLNS transport domain. The corresponding transport address is of type SnmpOSIAddress.')
snmpCONSDomain = ObjectIdentity((1, 3, 6, 1, 6, 1, 3))
if mibBuilder.loadTexts: snmpCONSDomain.setStatus('current')
if mibBuilder.loadTexts: snmpCONSDomain.setDescription('The SNMP over CONS transport domain. The corresponding transport address is of type SnmpOSIAddress.')


class SnmpOSIAddress(TextualConvention, OctetString):
    description = "Represents an OSI transport-address: octets contents encoding 1 length of NSAP 'n' as an unsigned-integer (either 0 or from 3 to 20) 2..(n+1) NSAP concrete binary representation (n+2)..m TSEL string of (up to 64) octets "
    status = 'current'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(1, 85)
    displayHint = "*1x:/1x:"

snmpDDPDomain = ObjectIdentity((1, 3, 6, 1, 6, 1, 4))
if mibBuilder.loadTexts: snmpDDPDomain.setStatus('current')
if mibBuilder.loadTexts: snmpDDPDomain.setDescription('The SNMP over DDP transport domain. The corresponding transport address is of type SnmpNBPAddress.')


class SnmpNBPAddress(OctetString, TextualConvention):
    description = "Represents an NBP name: octets contents encoding 1 length of object 'n' as an unsigned integer 2..(n+1) object string of (up to 32) octets n+2 length of type 'p' as an unsigned integer (n+3)..(n+2+p) type string of (up to 32) octets n+3+p length of zone 'q' as an unsigned integer (n+4+p)..(n+3+p+q) zone string of (up to 32) octets For comparison purposes, strings are case-insensitive. All strings may contain any octet other than 255 (hex ff)."
    status = 'current'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(3, 99)

snmpIPXDomain = ObjectIdentity((1, 3, 6, 1, 6, 1, 5))
if mibBuilder.loadTexts: snmpIPXDomain.setStatus('current')
if mibBuilder.loadTexts: snmpIPXDomain.setDescription('The SNMP over IPX transport domain. The corresponding transport address is of type SnmpIPXAddress.')
class SnmpIPXAddress(TextualConvention, OctetString):
    description = 'Represents an IPX address: octets contents encoding 1-4 network-number network-byte order 5-10 physical-address network-byte order 11-12 socket-number network-byte order '
    status = 'current'
    subtypeSpec = OctetString.subtypeSpec + ValueSizeConstraint(12, 12)
    displayHint = "4x.1x:1x:1x:1x:1x:1x.2d"
    fixedLength = 12

rfc1157Proxy = MibIdentifier((1, 3, 6, 1, 6, 2, 1))
rfc1157Domain = ObjectIdentity((1, 3, 6, 1, 6, 2, 1, 1))
if mibBuilder.loadTexts: rfc1157Domain.setStatus('deprecated')
if mibBuilder.loadTexts: rfc1157Domain.setDescription('The transport domain for SNMPv1 over UDP over IPv4. The corresponding transport address is of type SnmpUDPAddress.')

mibBuilder.exportSymbols("SNMPv2-TM", SnmpNBPAddress=SnmpNBPAddress, rfc1157Domain=rfc1157Domain, SnmpIPXAddress=SnmpIPXAddress, snmpUDPDomain=snmpUDPDomain, snmpCLNSDomain=snmpCLNSDomain, SnmpOSIAddress=SnmpOSIAddress, rfc1157Proxy=rfc1157Proxy, PYSNMP_MODULE_ID=snmpv2tm, snmpIPXDomain=snmpIPXDomain, snmpCONSDomain=snmpCONSDomain, snmpDDPDomain=snmpDDPDomain, SnmpUDPAddress=SnmpUDPAddress, snmpv2tm=snmpv2tm)
