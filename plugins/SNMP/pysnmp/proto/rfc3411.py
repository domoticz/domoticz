#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto import rfc1157, rfc1905

readClassPDUs = {
    rfc1157.GetRequestPDU.tagSet: 1,
    rfc1157.GetNextRequestPDU.tagSet: 1,
    rfc1905.GetRequestPDU.tagSet: 1,
    rfc1905.GetNextRequestPDU.tagSet: 1,
    rfc1905.GetBulkRequestPDU.tagSet: 1
}

writeClassPDUs = {
    rfc1157.SetRequestPDU.tagSet: 1,
    rfc1905.SetRequestPDU.tagSet: 1
}

responseClassPDUs = {
    rfc1157.GetResponsePDU.tagSet: 1,
    rfc1905.ResponsePDU.tagSet: 1,
    rfc1905.ReportPDU.tagSet: 1
}

notificationClassPDUs = {
    rfc1157.TrapPDU.tagSet: 1,
    rfc1905.SNMPv2TrapPDU.tagSet: 1,
    rfc1905.InformRequestPDU.tagSet: 1
}

internalClassPDUs = {
    rfc1905.ReportPDU.tagSet: 1
}

confirmedClassPDUs = {
    rfc1157.GetRequestPDU.tagSet: 1,
    rfc1157.GetNextRequestPDU.tagSet: 1,
    rfc1157.SetRequestPDU.tagSet: 1,
    rfc1905.GetRequestPDU.tagSet: 1,
    rfc1905.GetNextRequestPDU.tagSet: 1,
    rfc1905.GetBulkRequestPDU.tagSet: 1,
    rfc1905.SetRequestPDU.tagSet: 1,
    rfc1905.InformRequestPDU.tagSet: 1
}

unconfirmedClassPDUs = {
    rfc1157.GetResponsePDU.tagSet: 1,
    rfc1905.ResponsePDU.tagSet: 1,
    rfc1157.TrapPDU.tagSet: 1,
    rfc1905.ReportPDU.tagSet: 1,
    rfc1905.SNMPv2TrapPDU.tagSet: 1
}
