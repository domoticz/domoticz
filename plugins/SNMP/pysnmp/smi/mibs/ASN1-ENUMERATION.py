#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pyasn1.type import namedval

mibBuilder.exportSymbols(
    'ASN1-ENUMERATION',
    NamedValues=namedval.NamedValues
)
