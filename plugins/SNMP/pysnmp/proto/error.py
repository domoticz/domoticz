#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pyasn1.error import PyAsn1Error
from pysnmp.error import PySnmpError
from pysnmp import debug


class ProtocolError(PySnmpError, PyAsn1Error):
    pass


# SNMP v3 exceptions

class SnmpV3Error(ProtocolError):
    pass


class StatusInformation(SnmpV3Error):
    def __init__(self, **kwargs):
        SnmpV3Error.__init__(self)
        self.__errorIndication = kwargs
        debug.logger & (debug.flagDsp | debug.flagMP | debug.flagSM | debug.flagACL) and debug.logger(
            'StatusInformation: %s' % kwargs)

    def __str__(self):
        return str(self.__errorIndication)

    def __getitem__(self, key):
        return self.__errorIndication[key]

    def __contains__(self, key):
        return key in self.__errorIndication

    def get(self, key, defVal=None):
        return self.__errorIndication.get(key, defVal)


class CacheExpiredError(SnmpV3Error):
    pass


class InternalError(SnmpV3Error):
    pass


class MessageProcessingError(SnmpV3Error):
    pass


class RequestTimeout(SnmpV3Error):
    pass
