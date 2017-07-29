#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pyasn1.compat.octets import null

__all__ = ['ContextData']


class ContextData(object):
    """Creates UDP/IPv6 configuration entry and initialize socket API if needed.

    This object can be used by
    :py:class:`~pysnmp.hlapi.asyncore.AsyncCommandGenerator` or
    :py:class:`~pysnmp.hlapi.asyncore.AsyncNotificationOriginator`
    and their derevatives for forming SNMP PDU and also adding new entries to
    Local Configuration Datastore (LCD) in order to support SNMPv1/v2c with
    SNMPv3 interoperability.

    See :RFC:`3411#section-4.1` for SNMP Context details.

    Parameters
    ----------
    contextEngineId : str
        Uniquely identifies an SNMP entity that may realize an instance of
        a MIB with a particular contextName (:RFC:`3411#section-3.3.2`).
        More frequently than not, ContextEngineID is the same as
        authoritative SnmpEngineID, however if SNMP Engine serves multiple
        SNMP Entities, their ContextEngineIDs would be distinct.
        Default is authoritative SNMP Engine ID.
    contextName : str
        Used to name an instance of MIB (:RFC:`3411#section-3.3.3`).
        Default is empty string.

    Examples
    --------
    >>> from pysnmp.hlapi import ContextData
    >>> ContextData()
    ContextData(contextEngineId=None, contextName='')
    >>> ContextData(OctetString(hexValue='01020ABBA0'))
    ContextData(contextEngineId=OctetString(hexValue='01020abba0'), contextName='')
    >>> ContextData(contextName='mycontext')
    ContextData(contextEngineId=None, contextName='mycontext')

    """

    def __init__(self, contextEngineId=None, contextName=null):
        self.contextEngineId = contextEngineId
        self.contextName = contextName

    def __repr__(self):
        return '%s(contextEngineId=%r, contextName=%r)' % (
            self.__class__.__name__, self.contextEngineId, self.contextName
        )
