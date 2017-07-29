#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# PySNMP MIB module SNMPv2-CONF (http://pysnmp.sf.net)
# ASN.1 source http://mibs.snmplabs.com:80/asn1/SNMPv2-CONF
# Produced by pysmi-0.1.3 at Tue Apr 18 00:51:39 2017
# On host grommit.local platform Darwin version 16.4.0 by user ilya
# Using Python version 3.4.2 (v3.4.2:ab2c023a9432, Oct  5 2014, 20:42:22)
#
MibNode, = mibBuilder.importSymbols('SNMPv2-SMI', 'MibNode')


class ObjectGroup(MibNode):
    objects = ()
    description = ''

    def getObjects(self):
        return getattr(self, 'objects', ())

    def setObjects(self, *args):
        self.objects = args
        return self

    def getDescription(self):
        return getattr(self, 'description', '')

    def setDescription(self, v):
        self.description = v
        return self

    def asn1Print(self):
        return '\
OBJECT-GROUP\n\
  OBJECTS { %s }\n\
  DESCRIPTION \"%s\"\
' % (', '.join([x for x in self.getObjects()]), self.getDescription())


class NotificationGroup(MibNode):
    objects = ()
    description = ''

    def getObjects(self):
        return getattr(self, 'objects', ())

    def setObjects(self, *args):
        self.objects = args
        return self

    def getDescription(self):
        return getattr(self, 'description', '')

    def setDescription(self, v):
        self.description = v
        return self

    def asn1Print(self):
        return '\
NOTIFICATION-GROUP\n\
  NOTIFICATIONS { %s }\n\
  DESCRIPTION \"%s\"\
' % (', '.join([x for x in self.getObjects()]), self.getDescription())


class ModuleCompliance(MibNode):
    objects = ()
    description = ''

    def getObjects(self):
        return getattr(self, 'objects', ())

    def setObjects(self, *args):
        self.objects = args
        return self

    def getDescription(self):
        return getattr(self, 'description', '')

    def setDescription(self, v):
        self.description = v
        return self

    def asn1Print(self):
        return '\
MODULE-COMPLIANCE\n\
  OBJECT { %s } \n\
  DESCRIPTION \"%s\"\n\
' % (', '.join([x for x in self.getObjects()]), self.getDescription())


class AgentCapabilities(MibNode):
    description = ''

    def getDescription(self):
        return getattr(self, 'description', '')

    def setDescription(self, v):
        self.description = v
        return self

    def asn1Print(self):
        return '\
AGENT-CAPABILITIES\n\
  DESCRIPTION \"%s\"\n\
' % self.getDescription()


mibBuilder.exportSymbols('SNMPv2-CONF', ObjectGroup=ObjectGroup, NotificationGroup=NotificationGroup, ModuleCompliance=ModuleCompliance, AgentCapabilities=AgentCapabilities)
