#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pyasn1.type import univ, tag, constraint, namedtype
from pyasn1.error import PyAsn1Error
from pysnmp.proto import error

__all__ = ['Opaque', 'NetworkAddress', 'ObjectName', 'TimeTicks',
           'Counter', 'Gauge', 'IpAddress']


class IpAddress(univ.OctetString):
    tagSet = univ.OctetString.tagSet.tagImplicitly(
        tag.Tag(tag.tagClassApplication, tag.tagFormatSimple, 0x00)
    )
    subtypeSpec = univ.OctetString.subtypeSpec + constraint.ValueSizeConstraint(
        4, 4
    )

    def prettyIn(self, value):
        if isinstance(value, str) and len(value) != 4:
            try:
                value = [int(x) for x in value.split('.')]
            except:
                raise error.ProtocolError('Bad IP address syntax %s' % value)
        if len(value) != 4:
            raise error.ProtocolError('Bad IP address syntax')
        return univ.OctetString.prettyIn(self, value)

    def prettyOut(self, value):
        if value:
            return '.'.join(['%d' % x for x in self.__class__(value).asNumbers()])
        else:
            return ''


class Counter(univ.Integer):
    tagSet = univ.Integer.tagSet.tagImplicitly(
        tag.Tag(tag.tagClassApplication, tag.tagFormatSimple, 0x01)
    )
    subtypeSpec = univ.Integer.subtypeSpec + constraint.ValueRangeConstraint(
        0, 4294967295
    )


class NetworkAddress(univ.Choice):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('internet', IpAddress())
    )


class Gauge(univ.Integer):
    tagSet = univ.Integer.tagSet.tagImplicitly(
        tag.Tag(tag.tagClassApplication, tag.tagFormatSimple, 0x02)
    )
    subtypeSpec = univ.Integer.subtypeSpec + constraint.ValueRangeConstraint(
        0, 4294967295
    )


class TimeTicks(univ.Integer):
    tagSet = univ.Integer.tagSet.tagImplicitly(
        tag.Tag(tag.tagClassApplication, tag.tagFormatSimple, 0x03)
    )
    subtypeSpec = univ.Integer.subtypeSpec + constraint.ValueRangeConstraint(
        0, 4294967295
    )


class Opaque(univ.OctetString):
    tagSet = univ.OctetString.tagSet.tagImplicitly(
        tag.Tag(tag.tagClassApplication, tag.tagFormatSimple, 0x04)
    )


class ObjectName(univ.ObjectIdentifier):
    pass


class TypeCoercionHackMixIn:  # XXX keep this old-style class till pyasn1 types becomes new-style
    # Reduce ASN1 type check to simple tag check as SMIv2 objects may
    # not be constraints-compatible with those used in SNMP PDU.
    def _verifyComponent(self, idx, value, **kwargs):
        componentType = self._componentType
        if componentType:
            if idx >= len(componentType):
                raise PyAsn1Error('Component type error out of range')
            t = componentType[idx].getType()
            if not t.getTagSet().isSuperTagSetOf(value.getTagSet()):
                raise PyAsn1Error('Component type error %r vs %r' % (t, value))


class SimpleSyntax(TypeCoercionHackMixIn, univ.Choice):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('number', univ.Integer()),
        namedtype.NamedType('string', univ.OctetString()),
        namedtype.NamedType('object', univ.ObjectIdentifier()),
        namedtype.NamedType('empty', univ.Null())
    )


class ApplicationSyntax(TypeCoercionHackMixIn, univ.Choice):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('address', NetworkAddress()),
        namedtype.NamedType('counter', Counter()),
        namedtype.NamedType('gauge', Gauge()),
        namedtype.NamedType('ticks', TimeTicks()),
        namedtype.NamedType('arbitrary', Opaque())
    )


class ObjectSyntax(univ.Choice):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('simple', SimpleSyntax()),
        namedtype.NamedType('application-wide', ApplicationSyntax())
    )
