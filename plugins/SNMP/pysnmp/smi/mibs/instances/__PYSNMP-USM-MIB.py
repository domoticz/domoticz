#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
MibScalarInstance, = mibBuilder.importSymbols('SNMPv2-SMI', 'MibScalarInstance')

(pysnmpUsmDiscoverable,
 pysnmpUsmDiscovery) = mibBuilder.importSymbols(
    'PYSNMP-USM-MIB',
    'pysnmpUsmDiscoverable',
    'pysnmpUsmDiscovery'
)

__pysnmpUsmDiscoverable = MibScalarInstance(pysnmpUsmDiscoverable.name, (0,), pysnmpUsmDiscoverable.syntax)
__pysnmpUsmDiscovery = MibScalarInstance(pysnmpUsmDiscovery.name, (0,), pysnmpUsmDiscovery.syntax)

mibBuilder.exportSymbols(
    "__PYSNMP-USM-MIB",
    pysnmpUsmDiscoverable=__pysnmpUsmDiscoverable,
    pysnmpUsmDiscovery=__pysnmpUsmDiscovery
)
