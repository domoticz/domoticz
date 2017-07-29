#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto import errind, error
from pysnmp import debug


# rfc3415 3.2
# noinspection PyUnusedLocal
class Vacm(object):
    """Void Access Control Model"""
    accessModelID = 0

    def isAccessAllowed(self,
                        snmpEngine,
                        securityModel,
                        securityName,
                        securityLevel,
                        viewType,
                        contextName,
                        variableName):
        debug.logger & debug.flagACL and debug.logger(
            'isAccessAllowed: viewType %s for variableName %s - OK' % (viewType, variableName)
        )

        # rfc3415 3.2.5c
        return error.StatusInformation(errorIndication=errind.accessAllowed)
