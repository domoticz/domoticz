#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# PySNMP MIB module SNMP-USER-BASED-SM-3DES-MIB (http://pysnmp.sf.net)
# ASN.1 source http://mibs.snmplabs.com:80/asn1/SNMP-USER-BASED-SM-3DES-MIB
# Produced by pysmi-0.1.3 at Tue Apr 18 00:47:21 2017
# On host grommit.local platform Darwin version 16.4.0 by user ilya
# Using Python version 3.4.2 (v3.4.2:ab2c023a9432, Oct  5 2014, 20:42:22)
#
Integer, ObjectIdentifier, OctetString = mibBuilder.importSymbols("ASN1", "Integer", "ObjectIdentifier", "OctetString")
NamedValues, = mibBuilder.importSymbols("ASN1-ENUMERATION", "NamedValues")
ValueRangeConstraint, ConstraintsUnion, ValueSizeConstraint, SingleValueConstraint, ConstraintsIntersection = mibBuilder.importSymbols("ASN1-REFINEMENT", "ValueRangeConstraint", "ConstraintsUnion", "ValueSizeConstraint", "SingleValueConstraint", "ConstraintsIntersection")
snmpPrivProtocols, = mibBuilder.importSymbols("SNMP-FRAMEWORK-MIB", "snmpPrivProtocols")
ModuleCompliance, NotificationGroup = mibBuilder.importSymbols("SNMPv2-CONF", "ModuleCompliance", "NotificationGroup")
Bits, MibScalar, MibTable, MibTableRow, MibTableColumn, Gauge32, IpAddress, TimeTicks, Integer32, Counter64, NotificationType, MibIdentifier, Unsigned32, ObjectIdentity, snmpModules, ModuleIdentity, iso, Counter32 = mibBuilder.importSymbols("SNMPv2-SMI", "Bits", "MibScalar", "MibTable", "MibTableRow", "MibTableColumn", "Gauge32", "IpAddress", "TimeTicks", "Integer32", "Counter64", "NotificationType", "MibIdentifier", "Unsigned32", "ObjectIdentity", "snmpModules", "ModuleIdentity", "iso", "Counter32")
TextualConvention, AutonomousType, DisplayString = mibBuilder.importSymbols("SNMPv2-TC", "TextualConvention", "AutonomousType", "DisplayString")
snmpUsmMIB = ModuleIdentity((1, 3, 6, 1, 6, 3, 15))
if mibBuilder.loadTexts: snmpUsmMIB.setRevisions(('1999-10-06 00:00',))
if mibBuilder.loadTexts: snmpUsmMIB.setLastUpdated('9910060000Z')
if mibBuilder.loadTexts: snmpUsmMIB.setOrganization('SNMPv3 Working Group')
if mibBuilder.loadTexts: snmpUsmMIB.setContactInfo('WG-email: snmpv3@lists.tislabs.com Subscribe: majordomo@lists.tislabs.com In msg body: subscribe snmpv3 Chair: Russ Mundy NAI Labs postal: 3060 Washington Rd Glenwood MD 21738 USA email: mundy@tislabs.com phone: +1-443-259-2307 Co-editor: David Reeder NAI Labs postal: 3060 Washington Road (Route 97) Glenwood, MD 21738 USA email: dreeder@tislabs.com phone: +1-443-259-2348 Co-editor: Olafur Gudmundsson NAI Labs postal: 3060 Washington Road (Route 97) Glenwood, MD 21738 USA email: ogud@tislabs.com phone: +1-443-259-2389 ')
if mibBuilder.loadTexts: snmpUsmMIB.setDescription("Extension to the SNMP User-based Security Model to support Triple-DES EDE in 'Outside' CBC (cipher-block chaining) Mode. ")
usm3DESEDEPrivProtocol = ObjectIdentity((1, 3, 6, 1, 6, 3, 10, 1, 2, 3))
if mibBuilder.loadTexts: usm3DESEDEPrivProtocol.setStatus('current')
if mibBuilder.loadTexts: usm3DESEDEPrivProtocol.setDescription('The 3DES-EDE Symmetric Encryption Protocol.')
if mibBuilder.loadTexts: usm3DESEDEPrivProtocol.setReference('- Data Encryption Standard, National Institute of Standards and Technology. Federal Information Processing Standard (FIPS) Publication 46-3, (1999, pending approval). Will supersede FIPS Publication 46-2. - Data Encryption Algorithm, American National Standards Institute. ANSI X3.92-1981, (December, 1980). - DES Modes of Operation, National Institute of Standards and Technology. Federal Information Processing Standard (FIPS) Publication 81, (December, 1980). - Data Encryption Algorithm - Modes of Operation, American National Standards Institute. ANSI X3.106-1983, (May 1983). ')
mibBuilder.exportSymbols("SNMP-USER-BASED-SM-3DES-MIB", snmpUsmMIB=snmpUsmMIB, PYSNMP_MODULE_ID=snmpUsmMIB, usm3DESEDEPrivProtocol=usm3DESEDEPrivProtocol)
