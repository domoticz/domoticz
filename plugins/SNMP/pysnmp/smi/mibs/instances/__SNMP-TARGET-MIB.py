#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
MibScalarInstance, = mibBuilder.importSymbols(
    'SNMPv2-SMI',
    'MibScalarInstance'
)

(snmpTargetSpinLock,
 snmpUnavailableContexts,
 snmpUnknownContexts) = mibBuilder.importSymbols(
    'SNMP-TARGET-MIB',
    'snmpTargetSpinLock',
    'snmpUnavailableContexts',
    'snmpUnknownContexts'
)

__snmpTargetSpinLock = MibScalarInstance(snmpTargetSpinLock.name, (0,), snmpTargetSpinLock.syntax.clone(0))
__snmpUnavailableContexts = MibScalarInstance(snmpUnavailableContexts.name, (0,),
                                              snmpUnavailableContexts.syntax.clone(0))
__snmpUnknownContexts = MibScalarInstance(snmpUnknownContexts.name, (0,), snmpUnknownContexts.syntax.clone(0))

mibBuilder.exportSymbols(
    '__SNMP-TARGET-MIB',
    snmpTargetSpinLock=__snmpTargetSpinLock,
    snmpUnavailableContexts=__snmpUnavailableContexts,
    snmpUnknownContexts=__snmpUnknownContexts
)
