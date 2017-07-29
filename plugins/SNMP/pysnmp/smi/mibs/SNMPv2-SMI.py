#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
import traceback
from pysnmp.smi.indices import OidOrderedDict
from pysnmp.smi import exval, error
from pysnmp.proto import rfc1902
from pysnmp import cache, debug
from pyasn1.error import PyAsn1Error

Integer, ObjectIdentifier = mibBuilder.importSymbols(
    "ASN1", "Integer", "ObjectIdentifier"
)

(ConstraintsIntersection, ConstraintsUnion, SingleValueConstraint,
 ValueRangeConstraint, ValueSizeConstraint) = mibBuilder.importSymbols(
    "ASN1-REFINEMENT", "ConstraintsIntersection", "ConstraintsUnion",
    "SingleValueConstraint", "ValueRangeConstraint", "ValueSizeConstraint"
)

# syntax of objects

OctetString = rfc1902.OctetString
Bits = rfc1902.Bits
Integer32 = rfc1902.Integer32
IpAddress = rfc1902.IpAddress
Counter32 = rfc1902.Counter32
Gauge32 = rfc1902.Gauge32
Unsigned32 = rfc1902.Unsigned32
TimeTicks = rfc1902.TimeTicks
Opaque = rfc1902.Opaque
Counter64 = rfc1902.Counter64


class ExtUTCTime(OctetString):
    subtypeSpec = OctetString.subtypeSpec + ConstraintsUnion(ValueSizeConstraint(11, 11), ValueSizeConstraint(13, 13))


# MIB tree foundation class

class MibNode(object):
    label = ''

    def __init__(self, name):
        self.name = name

    def __repr__(self):
        return '%s(%r)' % (self.__class__.__name__, self.name)

    def getName(self):
        return self.name

    def getLabel(self):
        return self.label

    def setLabel(self, label):
        self.label = label
        return self

    def clone(self, name=None):
        myClone = self.__class__(self.name)
        if name is not None:
            myClone.name = name
        if self.label is not None:
            myClone.label = self.label
        return myClone


# definitions for information modules

class ModuleIdentity(MibNode):
    lastUpdated = ''
    organization = ''
    contactInfo = ''
    description = ''
    revisions = ()

    def getLastUpdated(self):
        return self.lastUpdated

    def setLastUpdated(self, v):
        self.lastUpdated = v
        return self

    def getOrganization(self):
        return self.organization

    def setOrganization(self, v):
        self.organization = v
        return self

    def getContactInfo(self):
        return self.contactInfo

    def setContactInfo(self, v):
        self.contactInfo = v
        return self

    def getDescription(self):
        return self.description

    def setDescription(self, v):
        self.description = v
        return self

    def getRevisions(self):
        return self.revisions

    def setRevisions(self, args):
        self.revisions = args
        return self

    def asn1Print(self):
        return """\
MODULE-IDENTITY
  LAST-UPDATED %s
  ORGANIZATION "%s"
  CONTACT-INFO "%s"
  DESCRIPTION "%s"
  %s""" % (self.getLastUpdated(),
           self.getOrganization(),
           self.getContactInfo(),
           self.getDescription(),
           ''.join(["REVISION \"%s\"\n" % x for x in self.getRevisions()]))


class ObjectIdentity(MibNode):
    status = 'current'
    description = ''
    reference = ''

    def getStatus(self):
        return self.status

    def setStatus(self, v):
        self.status = v
        return self

    def getDescription(self):
        return self.description

    def setDescription(self, v):
        self.description = v
        return self

    def getReference(self):
        return self.reference

    def setReference(self, v):
        self.reference = v
        return self

    def asn1Print(self):
        return """\
OBJECT-IDENTITY
  STATUS %s
  DESCRIPTION "%s"
  REFERENCE "%s" """ % (self.getStatus(),
                        self.getDescription(),
                        self.getReference())


# definition for objects

class NotificationType(MibNode):
    objects = ()
    status = 'current'
    description = ''
    revisions = ()

    def getObjects(self):
        return self.objects

    def setObjects(self, *args):
        self.objects = args
        return self

    def getStatus(self):
        return self.status

    def setStatus(self, v):
        self.status = v
        return self

    def getDescription(self):
        return self.description

    def setDescription(self, v):
        self.description = v
        return self

    def getRevisions(self):
        return self.revisions

    def setRevisions(self, args):
        self.revisions = args
        return self

    def asn1Print(self):
        return """\
NOTIFICATION-TYPE
  OBJECTS { %s }
  STATUS %s
  DESCRIPTION "%s"
  %s""" % (', '.join([x for x in self.getObjects()]),
           self.getStatus(),
           self.getDescription(),
           ''.join(["REVISION \"%s\"\n" % x for x in self.getRevisions()]))


class MibIdentifier(MibNode):
    @staticmethod
    def asn1Print():
        return 'OBJECT IDENTIFIER'


class ObjectType(MibNode):
    units = ''
    maxAccess = 'not-accessible'
    status = 'current'
    description = ''
    reference = ''

    def __init__(self, name, syntax=None):
        MibNode.__init__(self, name)
        self.syntax = syntax

    # XXX
    def __eq__(self, other):
        return self.syntax == other

    def __ne__(self, other):
        return self.syntax != other

    def __lt__(self, other):
        return self.syntax < other

    def __le__(self, other):
        return self.syntax <= other

    def __gt__(self, other):
        return self.syntax > other

    def __ge__(self, other):
        return self.syntax >= other

    def __repr__(self):
        return '%s(%r, %r)' % (
            self.__class__.__name__, self.name, self.syntax
        )

    def getSyntax(self):
        return self.syntax

    def setSyntax(self, v):
        self.syntax = v
        return self

    def getUnits(self):
        return self.units

    def setUnits(self, v):
        self.units = v
        return self

    def getMaxAccess(self):
        return self.maxAccess

    def setMaxAccess(self, v):
        self.maxAccess = v
        return self

    def getStatus(self):
        return self.status

    def setStatus(self, v):
        self.status = v
        return self

    def getDescription(self):
        return self.description

    def setDescription(self, v):
        self.description = v
        return self

    def getReference(self):
        return self.reference

    def setReference(self, v):
        self.reference = v
        return self

    def asn1Print(self):
        return """
OBJECT-TYPE
  SYNTAX %s
  UNITS "%s"
  MAX-ACCESS %s
  STATUS %s
  DESCRIPTION "%s"
  REFERENCE "%s" """ % (self.getSyntax().__class__.__name__,
                        self.getUnits(),
                        self.getMaxAccess(),
                        self.getStatus(),
                        self.getDescription(),
                        self.getReference())


class MibTree(ObjectType):
    branchVersionId = 0  # cnanges on tree structure change
    maxAccess = 'not-accessible'

    def __init__(self, name, syntax=None):
        ObjectType.__init__(self, name, syntax)
        self._vars = OidOrderedDict()

    # Subtrees registration

    def registerSubtrees(self, *subTrees):
        self.branchVersionId += 1
        for subTree in subTrees:
            if subTree.name in self._vars:
                raise error.SmiError(
                    'MIB subtree %s already registered at %s' % (subTree.name, self)
                )
            self._vars[subTree.name] = subTree

    def unregisterSubtrees(self, *names):
        self.branchVersionId += 1
        for name in names:
            # This may fail if you fill a table by exporting MibScalarInstances
            # but later drop them through SNMP.
            if name not in self._vars:
                raise error.SmiError(
                    'MIB subtree %s not registered at %s' % (name, self)
                )
            del self._vars[name]

    #
    # Tree traversal
    #
    # Missing branches are indicated by the NoSuchObjectError exception.
    # Although subtrees may indicate their missing branches by the
    # NoSuchInstanceError exception.
    #

    def getBranch(self, name, idx):
        """Return a branch of this tree where the 'name' OID may reside"""
        for keyLen in self._vars.getKeysLens():
            subName = name[:keyLen]
            if subName in self._vars:
                return self._vars[subName]

        raise error.NoSuchObjectError(name=name, idx=idx)

    def getNextBranch(self, name, idx=None):
        # Start from the beginning
        if self._vars:
            first = list(self._vars.keys())[0]
        else:
            first = ()
        if self._vars and name < first:
            return self._vars[first]
        else:
            try:
                return self._vars[self._vars.nextKey(name)]
            except KeyError:
                raise error.NoSuchObjectError(idx=idx, name=name)

    def getNode(self, name, idx=None):
        """Return tree node found by name"""
        if name == self.name:
            return self
        else:
            return self.getBranch(name, idx).getNode(name, idx)

    def getNextNode(self, name, idx=None):
        """Return tree node next to name"""
        try:
            nextNode = self.getBranch(name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            return self.getNextBranch(name, idx)
        else:
            try:
                return nextNode.getNextNode(name, idx)
            except (error.NoSuchInstanceError, error.NoSuchObjectError):
                try:
                    return self._vars[self._vars.nextKey(nextNode.name)]
                except KeyError:
                    raise error.NoSuchObjectError(idx=idx, name=name)

    # MIB instrumentation

    # Read operation

    def readTest(self, name, val, idx, acInfo):
        (acFun, acCtx) = acInfo
        if name == self.name:
            if acFun:
                if self.maxAccess not in ('readonly',
                                          'readwrite', 'readcreate') or \
                        acFun(name, self.syntax, idx, 'read', acCtx):
                    raise error.NoAccessError(idx=idx, name=name)
        else:
            try:
                node = self.getBranch(name, idx)
            except (error.NoSuchInstanceError, error.NoSuchObjectError):
                return  # missing object is not an error here
            else:
                node.readTest(name, val, idx, acInfo)

    def readGet(self, name, val, idx, acInfo):
        try:
            node = self.getBranch(name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            return name, exval.noSuchObject
        else:
            return node.readGet(name, val, idx, acInfo)

    # Read next operation is subtree-specific

    depthFirst, breadthFirst = 0, 1

    def readTestNext(self, name, val, idx, acInfo, oName=None):
        if oName is None:
            oName = name
            topOfTheMib = True
        else:
            topOfTheMib = False
        nextName = name
        direction = self.depthFirst
        while 1:  # XXX linear search here
            if direction == self.depthFirst:
                direction = self.breadthFirst
                try:
                    node = self.getBranch(nextName, idx)
                except (error.NoSuchInstanceError, error.NoSuchObjectError):
                    continue
            else:
                try:
                    node = self.getNextBranch(nextName, idx)
                except (error.NoSuchInstanceError, error.NoSuchObjectError):
                    if topOfTheMib:
                        return
                    raise
                direction = self.depthFirst
                nextName = node.name
            try:
                return node.readTestNext(nextName, val, idx, acInfo, oName)
            except (error.NoAccessError, error.NoSuchInstanceError, error.NoSuchObjectError):
                pass

    def readGetNext(self, name, val, idx, acInfo, oName=None):
        if oName is None:
            oName = name
            topOfTheMib = True
        else:
            topOfTheMib = False
        nextName = name
        direction = self.depthFirst
        while True:  # XXX linear search ahead!
            if direction == self.depthFirst:
                direction = self.breadthFirst
                try:
                    node = self.getBranch(nextName, idx)
                except (error.NoSuchInstanceError, error.NoSuchObjectError):
                    continue
            else:
                try:
                    node = self.getNextBranch(nextName, idx)
                except (error.NoSuchInstanceError, error.NoSuchObjectError):
                    if topOfTheMib:
                        return name, exval.endOfMib
                    raise
                direction = self.depthFirst
                nextName = node.name
            try:
                return node.readGetNext(nextName, val, idx, acInfo, oName)
            except (error.NoAccessError, error.NoSuchInstanceError, error.NoSuchObjectError):
                pass

    # Write operation

    def writeTest(self, name, val, idx, acInfo):
        acFun, acCtx = acInfo
        if name == self.name:
            # Make sure variable is writable
            if acFun:
                if self.maxAccess not in ('readwrite', 'readcreate') or \
                        acFun(name, self.syntax, idx, 'write', acCtx):
                    raise error.NotWritableError(idx=idx, name=name)
        else:
            node = self.getBranch(name, idx)
            node.writeTest(name, val, idx, acInfo)

    def writeCommit(self, name, val, idx, acInfo):
        self.getBranch(name, idx).writeCommit(name, val, idx, acInfo)

    def writeCleanup(self, name, val, idx, acInfo):
        self.branchVersionId += 1
        self.getBranch(name, idx).writeCleanup(name, val, idx, acInfo)

    def writeUndo(self, name, val, idx, acInfo):
        self.getBranch(name, idx).writeUndo(name, val, idx, acInfo)


class MibScalar(MibTree):
    """Scalar MIB variable. Implements access control checking."""
    maxAccess = 'readonly'

    #
    # Subtree traversal
    #
    # Missing branches are indicated by the NoSuchInstanceError exception.
    #

    def getBranch(self, name, idx):
        try:
            return MibTree.getBranch(self, name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def getNextBranch(self, name, idx=None):
        try:
            return MibTree.getNextBranch(self, name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def getNode(self, name, idx=None):
        try:
            return MibTree.getNode(self, name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def getNextNode(self, name, idx=None):
        try:
            return MibTree.getNextNode(self, name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            raise error.NoSuchInstanceError(idx=idx, name=name)

    # MIB instrumentation methods

    # Read operation

    def readTest(self, name, val, idx, acInfo):
        (acFun, acCtx) = acInfo
        if name == self.name:
            raise error.NoAccessError(idx=idx, name=name)
        if acFun:
            if self.maxAccess not in ('readonly', 'readwrite',
                                      'readcreate') or \
                    acFun(name, self.syntax, idx, 'read', acCtx):
                raise error.NoAccessError(idx=idx, name=name)
        MibTree.readTest(self, name, val, idx, acInfo)

    def readGet(self, name, val, idx, acInfo):
        try:
            node = self.getBranch(name, idx)
        except error.NoSuchInstanceError:
            return name, exval.noSuchInstance
        else:
            return node.readGet(name, val, idx, acInfo)

    def readTestNext(self, name, val, idx, acInfo, oName=None):
        (acFun, acCtx) = acInfo
        if acFun:
            if self.maxAccess not in ('readonly', 'readwrite',
                                      'readcreate') or \
                    acFun(name, self.syntax, idx, 'read', acCtx):
                raise error.NoAccessError(idx=idx, name=name)
        MibTree.readTestNext(self, name, val, idx, acInfo, oName)

    def readGetNext(self, name, val, idx, acInfo, oName=None):
        (acFun, acCtx) = acInfo
        # have to duplicate AC here as *Next code above treats
        # noAccess as a noSuchObject at the Test stage, goes on
        # to Reading
        if acFun:
            if self.maxAccess not in ('readonly', 'readwrite',
                                      'readcreate') or \
                    acFun(name, self.syntax, idx, 'read', acCtx):
                raise error.NoAccessError(idx=idx, name=name)
        return MibTree.readGetNext(self, name, val, idx, acInfo, oName)

    # Two-phase commit implementation

    def writeTest(self, name, val, idx, acInfo):
        acFun, acCtx = acInfo
        if name == self.name:
            raise error.NoAccessError(idx=idx, name=name)
        if acFun:
            if self.maxAccess not in ('readwrite', 'readcreate') or \
                    acFun(name, self.syntax, idx, 'write', acCtx):
                raise error.NotWritableError(idx=idx, name=name)
        MibTree.writeTest(self, name, val, idx, acInfo)


class MibScalarInstance(MibTree):
    """Scalar MIB variable instance. Implements read/write operations."""

    def __init__(self, typeName, instId, syntax):
        MibTree.__init__(self, typeName + instId, syntax)
        self.typeName = typeName
        self.instId = instId
        self.__oldSyntax = None

    #
    # Managed object value access methods
    #

    # noinspection PyUnusedLocal
    def getValue(self, name, idx):
        debug.logger & debug.flagIns and debug.logger('getValue: returning %r for %s' % (self.syntax, self.name))
        return self.syntax.clone()

    def setValue(self, value, name, idx):
        try:
            if hasattr(self.syntax, 'setValue'):
                return self.syntax.setValue(value)
            else:
                return self.syntax.clone(value)
        except PyAsn1Error:
            exc_t, exc_v, exc_tb = sys.exc_info()
            debug.logger & debug.flagIns and debug.logger('setValue: %s=%r failed with traceback %s' % (
                self.name, value, traceback.format_exception(exc_t, exc_v, exc_tb)))
            if isinstance(exc_v, error.TableRowManagement):
                raise exc_v
            else:
                raise error.WrongValueError(idx=idx, name=name, msg=exc_v)

    #
    # Subtree traversal
    #
    # Missing branches are indicated by the NoSuchInstanceError exception.
    #

    def getBranch(self, name, idx):
        try:
            return MibTree.getBranch(self, name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def getNextBranch(self, name, idx=None):
        try:
            return MibTree.getNextBranch(self, name, idx)
        except (error.NoSuchInstanceError, error.NoSuchObjectError):
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def getNode(self, name, idx=None):
        # Recursion terminator
        if name == self.name:
            return self
        raise error.NoSuchInstanceError(idx=idx, name=name)

    def getNextNode(self, name, idx=None):
        raise error.NoSuchInstanceError(idx=idx, name=name)

    # MIB instrumentation methods

    # Read operation

    def readTest(self, name, val, idx, acInfo):
        if name != self.name:
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def readGet(self, name, val, idx, acInfo):
        # Return current variable (name, value)
        if name == self.name:
            debug.logger & debug.flagIns and debug.logger('readGet: %s=%r' % (self.name, self.syntax))
            return self.name, self.getValue(name, idx)
        else:
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def readTestNext(self, name, val, idx, acInfo, oName=None):
        if name != self.name or name <= oName:
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def readGetNext(self, name, val, idx, acInfo, oName=None):
        if name == self.name and name > oName:
            debug.logger & debug.flagIns and debug.logger('readGetNext: %s=%r' % (self.name, self.syntax))
            return self.readGet(name, val, idx, acInfo)
        else:
            raise error.NoSuchInstanceError(idx=idx, name=name)

    # Write operation: two-phase commit

    # noinspection PyAttributeOutsideInit
    def writeTest(self, name, val, idx, acInfo):
        # Make sure write's allowed
        if name == self.name:
            try:
                self.__newSyntax = self.setValue(val, name, idx)
            except error.MibOperationError:
                # SMI exceptions may carry additional content
                why = sys.exc_info()[1]
                if 'syntax' in why:
                    self.__newSyntax = why['syntax']
                    raise why
                else:
                    raise error.WrongValueError(idx=idx, name=name, msg=sys.exc_info()[1])
        else:
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def writeCommit(self, name, val, idx, acInfo):
        # Backup original value
        if self.__oldSyntax is None:
            self.__oldSyntax = self.syntax
        # Commit new value
        self.syntax = self.__newSyntax

    # noinspection PyAttributeOutsideInit
    def writeCleanup(self, name, val, idx, acInfo):
        self.branchVersionId += 1
        debug.logger & debug.flagIns and debug.logger('writeCleanup: %s=%r' % (name, val))
        # Drop previous value
        self.__newSyntax = self.__oldSyntax = None

    # noinspection PyAttributeOutsideInit
    def writeUndo(self, name, val, idx, acInfo):
        # Revive previous value
        self.syntax = self.__oldSyntax
        self.__newSyntax = self.__oldSyntax = None

    # Table column instance specifics

    # Create operation

    # noinspection PyUnusedLocal,PyAttributeOutsideInit
    def createTest(self, name, val, idx, acInfo):
        if name == self.name:
            try:
                self.__newSyntax = self.setValue(val, name, idx)
            except error.MibOperationError:
                # SMI exceptions may carry additional content
                why = sys.exc_info()[1]
                if 'syntax' in why:
                    self.__newSyntax = why['syntax']
                else:
                    raise error.WrongValueError(idx=idx, name=name, msg=sys.exc_info()[1])
        else:
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def createCommit(self, name, val, idx, acInfo):
        if val is not None:
            self.writeCommit(name, val, idx, acInfo)

    def createCleanup(self, name, val, idx, acInfo):
        self.branchVersionId += 1
        debug.logger & debug.flagIns and debug.logger('createCleanup: %s=%r' % (name, val))
        if val is not None:
            self.writeCleanup(name, val, idx, acInfo)

    def createUndo(self, name, val, idx, acInfo):
        if val is not None:
            self.writeUndo(name, val, idx, acInfo)

    # Destroy operation

    # noinspection PyUnusedLocal,PyAttributeOutsideInit
    def destroyTest(self, name, val, idx, acInfo):
        if name == self.name:
            try:
                self.__newSyntax = self.setValue(val, name, idx)
            except error.MibOperationError:
                # SMI exceptions may carry additional content
                why = sys.exc_info()[1]
                if 'syntax' in why:
                    self.__newSyntax = why['syntax']
        else:
            raise error.NoSuchInstanceError(idx=idx, name=name)

    def destroyCommit(self, name, val, idx, acInfo):
        pass

    # noinspection PyUnusedLocal
    def destroyCleanup(self, name, val, idx, acInfo):
        self.branchVersionId += 1

    def destroyUndo(self, name, val, idx, acInfo):
        pass


# Conceptual table classes

class MibTableColumn(MibScalar):
    """MIB table column. Manages a set of column instance variables"""
    protoInstance = MibScalarInstance

    def __init__(self, name, syntax):
        MibScalar.__init__(self, name, syntax)
        self.__createdInstances = {}
        self.__destroyedInstances = {}
        self.__rowOpWanted = {}

    #
    # Subtree traversal
    #
    # Missing leaves are indicated by the NoSuchInstanceError exception.
    #

    def getBranch(self, name, idx):
        if name in self._vars:
            return self._vars[name]
        raise error.NoSuchInstanceError(name=name, idx=idx)

    def setProtoInstance(self, protoInstance):
        self.protoInstance = protoInstance

    # Column creation (this should probably be converted into some state
    # machine for clarity). Also, it might be a good idea to inidicate
    # defaulted cols creation in a clearer way than just a val == None.

    def createTest(self, name, val, idx, acInfo):
        (acFun, acCtx) = acInfo
        # Make sure creation allowed, create a new column instance but
        # do not replace the old one
        if name == self.name:
            raise error.NoAccessError(idx=idx, name=name)
        if acFun:
            if val is not None and self.maxAccess != 'readcreate' or \
                    acFun(name, self.syntax, idx, 'write', acCtx):
                debug.logger & debug.flagACL and debug.logger(
                    'createTest: %s=%r %s at %s' % (name, val, self.maxAccess, self.name))
                raise error.NoCreationError(idx=idx, name=name)
        # Create instances if either it does not yet exist (row creation)
        # or a value is passed (multiple OIDs in SET PDU)
        if val is None and name in self.__createdInstances:
            return
        self.__createdInstances[name] = self.protoInstance(
            self.name, name[len(self.name):], self.syntax.clone()
        )
        self.__createdInstances[name].createTest(name, val, idx, acInfo)

    def createCommit(self, name, val, idx, acInfo):
        # Commit new instance value
        if name in self._vars:  # XXX
            if name in self.__createdInstances:
                self._vars[name].createCommit(name, val, idx, acInfo)
            return
        self.__createdInstances[name].createCommit(name, val, idx, acInfo)
        # ...commit new column instance
        self._vars[name], self.__createdInstances[name] = \
            self.__createdInstances[name], self._vars.get(name)

    def createCleanup(self, name, val, idx, acInfo):
        # Drop previous column instance
        self.branchVersionId += 1
        if name in self.__createdInstances:
            if self.__createdInstances[name] is not None:
                self.__createdInstances[name].createCleanup(name, val, idx,
                                                            acInfo)
            del self.__createdInstances[name]
        elif name in self._vars:
            self._vars[name].createCleanup(name, val, idx, acInfo)

    def createUndo(self, name, val, idx, acInfo):
        # Set back previous column instance, drop the new one
        if name in self.__createdInstances:
            self._vars[name] = self.__createdInstances[name]
            del self.__createdInstances[name]
            # Remove new instance on rollback
            if self._vars[name] is None:
                del self._vars[name]
            else:
                # Catch half-created instances (hackerish)
                try:
                    self._vars[name] == 0
                except PyAsn1Error:
                    del self._vars[name]
                else:
                    self._vars[name].createUndo(name, val, idx, acInfo)

    # Column destruction

    def destroyTest(self, name, val, idx, acInfo):
        (acFun, acCtx) = acInfo
        # Make sure destruction is allowed
        if name == self.name:
            raise error.NoAccessError(idx=idx, name=name)
        if name not in self._vars:
            return
        if acFun:
            if val is not None and self.maxAccess != 'readcreate' or \
                    acFun(name, self.syntax, idx, 'write', acCtx):
                raise error.NoAccessError(idx=idx, name=name)
        self._vars[name].destroyTest(name, val, idx, acInfo)

    def destroyCommit(self, name, val, idx, acInfo):
        # Make a copy of column instance and take it off the tree
        if name in self._vars:
            self._vars[name].destroyCommit(name, val, idx, acInfo)
            self.__destroyedInstances[name] = self._vars[name]
            del self._vars[name]

    def destroyCleanup(self, name, val, idx, acInfo):
        # Drop instance copy
        self.branchVersionId += 1
        if name in self.__destroyedInstances:
            self.__destroyedInstances[name].destroyCleanup(name, val,
                                                           idx, acInfo)
            debug.logger & debug.flagIns and debug.logger('destroyCleanup: %s=%r' % (name, val))
            del self.__destroyedInstances[name]

    def destroyUndo(self, name, val, idx, acInfo):
        # Set back column instance
        if name in self.__destroyedInstances:
            self._vars[name] = self.__destroyedInstances[name]
            self._vars[name].destroyUndo(name, val, idx, acInfo)
            del self.__destroyedInstances[name]

    # Set/modify column

    def writeTest(self, name, val, idx, acInfo):
        # Besides common checks, request row creation on no-instance
        try:
            # First try the instance
            MibScalar.writeTest(self, name, val, idx, acInfo)
        # ...otherwise proceed with creating new column
        except (error.NoSuchInstanceError, error.RowCreationWanted):
            excValue = sys.exc_info()[1]
            if isinstance(excValue, error.RowCreationWanted):
                self.__rowOpWanted[name] = excValue
            else:
                self.__rowOpWanted[name] = error.RowCreationWanted()
            self.createTest(name, val, idx, acInfo)
        except error.RowDestructionWanted:
            self.__rowOpWanted[name] = error.RowDestructionWanted()
            self.destroyTest(name, val, idx, acInfo)
        if name in self.__rowOpWanted:
            debug.logger & debug.flagIns and debug.logger(
                '%s flagged by %s=%r, exception %s' % (self.__rowOpWanted[name], name, val, sys.exc_info()[1]))
            raise self.__rowOpWanted[name]

    def __delegateWrite(self, subAction, name, val, idx, acInfo):
        if name not in self.__rowOpWanted:
            getattr(MibScalar, 'write' + subAction)(self, name, val, idx, acInfo)
            return
        if isinstance(self.__rowOpWanted[name], error.RowCreationWanted):
            getattr(self, 'create' + subAction)(name, val, idx, acInfo)
        if isinstance(self.__rowOpWanted[name], error.RowDestructionWanted):
            getattr(self, 'destroy' + subAction)(name, val, idx, acInfo)

    def writeCommit(self, name, val, idx, acInfo):
        self.__delegateWrite('Commit', name, val, idx, acInfo)
        if name in self.__rowOpWanted:
            raise self.__rowOpWanted[name]

    def writeCleanup(self, name, val, idx, acInfo):
        self.branchVersionId += 1
        self.__delegateWrite('Cleanup', name, val, idx, acInfo)
        if name in self.__rowOpWanted:
            e = self.__rowOpWanted[name]
            del self.__rowOpWanted[name]
            debug.logger & debug.flagIns and debug.logger('%s dropped by %s=%r' % (e, name, val))
            raise e

    def writeUndo(self, name, val, idx, acInfo):
        if name in self.__rowOpWanted:
            self.__rowOpWanted[name] = error.RowDestructionWanted()
        self.__delegateWrite('Undo', name, val, idx, acInfo)
        if name in self.__rowOpWanted:
            e = self.__rowOpWanted[name]
            del self.__rowOpWanted[name]
            debug.logger & debug.flagIns and debug.logger('%s dropped by %s=%r' % (e, name, val))
            raise e


class MibTableRow(MibTree):
    """MIB table row (SMI 'Entry'). Manages a set of table columns.
       Implements row creation/destruction.
    """

    def __init__(self, name):
        MibTree.__init__(self, name)
        self.__idToIdxCache = cache.Cache()
        self.__idxToIdCache = cache.Cache()
        self.indexNames = ()
        self.augmentingRows = {}

    # Table indices resolution. Handle almost all possible rfc1902 types
    # explicitly rather than by means of isSuperTypeOf() method because
    # some subtypes may be implicitly tagged what renders base tag
    # unavailable.

    __intBaseTag = Integer.tagSet.getBaseTag()
    __strBaseTag = OctetString.tagSet.getBaseTag()
    __oidBaseTag = ObjectIdentifier.tagSet.getBaseTag()
    __ipaddrTagSet = IpAddress.tagSet
    __bitsBaseTag = Bits.tagSet.getBaseTag()

    def setFromName(self, obj, value, impliedFlag=None, parentIndices=None):
        if not value:
            raise error.SmiError('Short OID for index %r' % (obj,))
        value = tuple(value)  # possible ObjectIdentifiers
        if hasattr(obj, 'cloneFromName'):
            return obj.cloneFromName(value, impliedFlag, parentRow=self, parentIndices=parentIndices)
        baseTag = obj.getTagSet().getBaseTag()
        if baseTag == self.__intBaseTag:
            return obj.clone(value[0]), value[1:]
        elif self.__ipaddrTagSet.isSuperTagSetOf(obj.getTagSet()):
            return obj.clone('.'.join([str(x) for x in value[:4]])), value[4:]
        elif baseTag == self.__strBaseTag:
            # rfc1902, 7.7
            if impliedFlag:
                return obj.clone(value), ()
            elif obj.isFixedLength():
                l = obj.getFixedLength()
                return obj.clone(value[:l]), value[l:]
            else:
                return obj.clone(value[1:value[0] + 1]), value[value[0] + 1:]
        elif baseTag == self.__oidBaseTag:
            if impliedFlag:
                return obj.clone(value), ()
            else:
                return obj.clone(value[1:value[0] + 1]), value[value[0] + 1:]
        # rfc2578, 7.1
        elif baseTag == self.__bitsBaseTag:
            return obj.clone(value[1:value[0] + 1]), value[value[0] + 1:]
        else:
            raise error.SmiError('Unknown value type for index %r' % (obj,))

    def getAsName(self, obj, impliedFlag=None, parentIndices=None):
        if hasattr(obj, 'cloneAsName'):
            return obj.cloneAsName(impliedFlag, parentRow=self, parentIndices=parentIndices)
        baseTag = obj.getTagSet().getBaseTag()
        if baseTag == self.__intBaseTag:
            # noinspection PyRedundantParentheses
            return (int(obj),)
        elif self.__ipaddrTagSet.isSuperTagSetOf(obj.getTagSet()):
            return obj.asNumbers()
        elif baseTag == self.__strBaseTag:
            if impliedFlag or obj.isFixedLength():
                initial = ()
            else:
                initial = (len(obj),)
            return initial + obj.asNumbers()
        elif baseTag == self.__oidBaseTag:
            if impliedFlag:
                return tuple(obj)
            else:
                return (len(self.name),) + tuple(obj)
        # rfc2578, 7.1
        elif baseTag == self.__bitsBaseTag:
            return (len(obj),) + obj.asNumbers()
        else:
            raise error.SmiError('Unknown value type for index %r' % (obj,))

    # Fate sharing mechanics

    def announceManagementEvent(self, action, name, val, idx, acInfo):
        # Convert OID suffix into index vals
        instId = name[len(self.name) + 1:]
        baseIndices = []
        indices = []
        for impliedFlag, modName, symName in self.indexNames:
            mibObj, = mibBuilder.importSymbols(modName, symName)
            syntax, instId = self.setFromName(mibObj.syntax, instId,
                                              impliedFlag, indices)

            if self.name == mibObj.name[:-1]:
                baseIndices.append((mibObj.name, syntax))

            indices.append(syntax)

        if instId:
            raise error.SmiError('Excessive instance identifier sub-OIDs left at %s: %s' % (self, instId))

        if not baseIndices:
            return

        for modName, mibSym in self.augmentingRows.keys():
            mibObj, = mibBuilder.importSymbols(modName, mibSym)
            debug.logger & debug.flagIns and debug.logger('announceManagementEvent %s to %s' % (action, mibObj))
            mibObj.receiveManagementEvent(
                action, baseIndices, val, idx, acInfo
            )

    def receiveManagementEvent(self, action, baseIndices, val, idx, acInfo):
        # The default implementation supports one-to-one rows dependency
        newSuffix = ()
        # Resolve indices intersection
        for impliedFlag, modName, symName in self.indexNames:
            mibObj, = mibBuilder.importSymbols(modName, symName)
            parentIndices = []
            for name, syntax in baseIndices:
                if name == mibObj.name:
                    newSuffix += self.getAsName(syntax, impliedFlag, parentIndices)
                parentIndices.append(syntax)

        if newSuffix:
            debug.logger & debug.flagIns and debug.logger(
                'receiveManagementEvent %s for suffix %s' % (action, newSuffix))
            self.__manageColumns(action, (), newSuffix, val, idx, acInfo)

    def registerAugmentions(self, *names):
        for modName, symName in names:
            if (modName, symName) in self.augmentingRows:
                raise error.SmiError(
                    'Row %s already augmented by %s::%s' % (self.name, modName, symName)
                )
            self.augmentingRows[(modName, symName)] = 1
        return self

    def setIndexNames(self, *names):
        for name in names:
            self.indexNames = self.indexNames + (name,)
        return self

    def getIndexNames(self):
        return self.indexNames

    def __manageColumns(self, action, excludeName, nameSuffix,
                        val, idx, acInfo):
        # Build a map of index names and values for automatic initialization
        indexVals = {}
        instId = nameSuffix
        indices = []
        for impliedFlag, modName, symName in self.indexNames:
            mibObj, = mibBuilder.importSymbols(modName, symName)
            syntax, instId = self.setFromName(mibObj.syntax, instId,
                                              impliedFlag, indices)
            indexVals[mibObj.name] = syntax
            indices.append(syntax)

        for name, var in self._vars.items():
            if name == excludeName:
                continue

            if name in indexVals:
                getattr(var, action)(name + nameSuffix, indexVals[name], idx,
                                     (None, None))
            else:
                getattr(var, action)(name + nameSuffix, val, idx, acInfo)

            debug.logger & debug.flagIns and debug.logger('__manageColumns: action %s name %s suffix %s %svalue %r' % (
                action, name, nameSuffix, name in indexVals and "index " or "", indexVals.get(name, val)))

    def __delegate(self, subAction, name, val, idx, acInfo):
        # Relay operation request to column, expect row operation request.
        rowIsActive = False
        try:
            getattr(self.getBranch(name, idx), 'write' + subAction)(
                name, val, idx, acInfo
            )

        except error.RowCreationWanted:
            self.__manageColumns(
                'create' + subAction, name[:len(self.name) + 1],
                name[len(self.name) + 1:], None, idx, acInfo
            )

            self.announceManagementEvent(
                'create' + subAction, name, None, idx, acInfo
            )

            # watch for RowStatus == 'stActive'
            rowIsActive = sys.exc_info()[1].get('syntax', 0) == 1

        except error.RowDestructionWanted:
            self.__manageColumns(
                'destroy' + subAction, name[:len(self.name) + 1],
                name[len(self.name) + 1:], None, idx, acInfo
            )

            self.announceManagementEvent(
                'destroy' + subAction, name, None, idx, acInfo
            )

        return rowIsActive

    def writeTest(self, name, val, idx, acInfo):
        self.__delegate('Test', name, val, idx, acInfo)

    def writeCommit(self, name, val, idx, acInfo):
        rowIsActive = self.__delegate('Commit', name, val, idx, acInfo)
        if rowIsActive:
            for mibNode in self._vars.values():
                colNode = mibNode.getNode(mibNode.name + name[len(self.name) + 1:])
                if not colNode.syntax.hasValue():
                    raise error.InconsistentValueError(msg='Row consistency check failed for %r' % colNode)

    def writeCleanup(self, name, val, idx, acInfo):
        self.branchVersionId += 1
        self.__delegate('Cleanup', name, val, idx, acInfo)

    def writeUndo(self, name, val, idx, acInfo):
        self.__delegate('Undo', name, val, idx, acInfo)

    # Table row management

    # Table row access by instance name

    def getInstName(self, colId, instId):
        return self.name + (colId,) + instId

    # Table index management

    def getIndicesFromInstId(self, instId):
        """Return index values for instance identification"""
        if instId in self.__idToIdxCache:
            return self.__idToIdxCache[instId]

        indices = []
        for impliedFlag, modName, symName in self.indexNames:
            mibObj, = mibBuilder.importSymbols(modName, symName)
            try:
                syntax, instId = self.setFromName(mibObj.syntax, instId, impliedFlag, indices)
            except PyAsn1Error:
                debug.logger & debug.flagIns and debug.logger('error resolving table indices at %s, %s: %s' % (self.__class__.__name__, instId, sys.exc_info()[1]))
                indices = [instId]
                instId = ()
                break

            indices.append(syntax)  # to avoid cyclic refs

        if instId:
            raise error.SmiError(
                'Excessive instance identifier sub-OIDs left at %s: %s' %
                (self, instId)
            )

        indices = tuple(indices)
        self.__idToIdxCache[instId] = indices

        return indices

    def getInstIdFromIndices(self, *indices):
        """Return column instance identification from indices"""
        if indices in self.__idxToIdCache:
            return self.__idxToIdCache[indices]
        idx = 0
        instId = ()
        parentIndices = []
        for impliedFlag, modName, symName in self.indexNames:
            if idx >= len(indices):
                break
            mibObj, = mibBuilder.importSymbols(modName, symName)
            syntax = mibObj.syntax.clone(indices[idx])
            instId += self.getAsName(syntax, impliedFlag, parentIndices)
            parentIndices.append(syntax)
            idx += 1
        self.__idxToIdCache[indices] = instId
        return instId

    # Table access by index

    def getInstNameByIndex(self, colId, *indices):
        """Build column instance name from components"""
        return self.name + (colId,) + self.getInstIdFromIndices(*indices)

    def getInstNamesByIndex(self, *indices):
        """Build column instance names from indices"""
        instNames = []
        for columnName in self._vars.keys():
            instNames.append(
                self.getInstNameByIndex(*(columnName[-1],) + indices)
            )

        return tuple(instNames)


class MibTable(MibTree):
    """MIB table. Manages a set of TableRow's"""

    def __init__(self, name):
        MibTree.__init__(self, name)


zeroDotZero = ObjectIdentity((0, 0))

# OID tree
itu_t = MibTree((0,)).setLabel('itu-t')
iso = MibTree((1,))
joint_iso_itu_t = MibTree((2,)).setLabel('joint-iso-itu-t')
org = MibIdentifier(iso.name + (3,))
dod = MibIdentifier(org.name + (6,))
internet = MibIdentifier(dod.name + (1,))
directory = MibIdentifier(internet.name + (1,))
mgmt = MibIdentifier(internet.name + (2,))
mib_2 = MibIdentifier(mgmt.name + (1,)).setLabel('mib-2')
transmission = MibIdentifier(mib_2.name + (10,))
experimental = MibIdentifier(internet.name + (3,))
private = MibIdentifier(internet.name + (4,))
enterprises = MibIdentifier(private.name + (1,))
security = MibIdentifier(internet.name + (5,))
snmpV2 = MibIdentifier(internet.name + (6,))

snmpDomains = MibIdentifier(snmpV2.name + (1,))
snmpProxys = MibIdentifier(snmpV2.name + (2,))
snmpModules = MibIdentifier(snmpV2.name + (3,))

mibBuilder.exportSymbols(
    'SNMPv2-SMI', MibNode=MibNode,
    Integer32=Integer32, Bits=Bits, IpAddress=IpAddress,
    Counter32=Counter32, Gauge32=Gauge32, Unsigned32=Unsigned32,
    TimeTicks=TimeTicks, Opaque=Opaque, Counter64=Counter64,
    ExtUTCTime=ExtUTCTime,
    ModuleIdentity=ModuleIdentity, ObjectIdentity=ObjectIdentity,
    NotificationType=NotificationType, MibScalar=MibScalar,
    MibScalarInstance=MibScalarInstance,
    MibIdentifier=MibIdentifier, MibTree=MibTree,
    MibTableColumn=MibTableColumn, MibTableRow=MibTableRow,
    MibTable=MibTable, zeroDotZero=zeroDotZero,
    itu_t=itu_t, iso=iso, joint_iso_itu_t=joint_iso_itu_t, org=org, dod=dod,
    internet=internet, directory=directory, mgmt=mgmt, mib_2=mib_2,
    transmission=transmission, experimental=experimental, private=private,
    enterprises=enterprises, security=security, snmpV2=snmpV2,
    snmpDomains=snmpDomains, snmpProxys=snmpProxys, snmpModules=snmpModules
)

# XXX
# getAsName/setFromName goes out of MibRow?
# revisit getNextNode() -- needs optimization
