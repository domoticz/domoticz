#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto.api import v1, v2c, verdec

# Protocol versions
protoVersion1 = 0
protoVersion2c = 1
protoModules = {protoVersion1: v1, protoVersion2c: v2c}

decodeMessageVersion = verdec.decodeMessageVersion
