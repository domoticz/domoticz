#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# PySNMP MIB module RFC1158-MIB (http://pysnmp.sf.net)
# ASN.1 source http://mibs.snmplabs.com:80/asn1/RFC1158-MIB
# Produced by pysmi-0.1.3 at Mon Apr 17 12:12:07 2017
# On host grommit.local platform Darwin version 16.4.0 by user ilya
# Using Python version 3.4.2 (v3.4.2:ab2c023a9432, Oct  5 2014, 20:42:22)
#
# It is a stripped version of MIB that contains only symbols that is
# unique to SMIv1 and have no analogues in SMIv2
#
Integer32, MibScalar, MibTable, MibTableRow, MibTableColumn, TimeTicks, iso, Gauge32, MibIdentifier, Bits, Counter32 = mibBuilder.importSymbols("SNMPv2-SMI", "Integer32", "MibScalar", "MibTable", "MibTableRow", "MibTableColumn", "TimeTicks", "iso", "Gauge32", "MibIdentifier", "Bits","Counter32")
snmpInBadTypes = MibScalar((1, 3, 6, 1, 2, 1, 11, 7), Counter32()).setMaxAccess("readonly")
if mibBuilder.loadTexts: snmpInBadTypes.setStatus('mandatory')
snmpOutReadOnlys = MibScalar((1, 3, 6, 1, 2, 1, 11, 23), Counter32()).setMaxAccess("readonly")
if mibBuilder.loadTexts: snmpInReadOnlys.setStatus('mandatory')
mibBuilder.exportSymbols("RFC1158-MIB", snmpOutReadOnlys=snmpOutReadOnlys, snmpInBadTypes=snmpInBadTypes)
