#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import sys
from pysnmp.proto import rfc1902, rfc1905
from pysnmp.proto.api import v2c
from pysnmp.smi.builder import ZipMibSource
from pysnmp.smi.compiler import addMibCompiler
from pysnmp.smi.error import SmiError
from pyasn1.type.base import AbstractSimpleAsn1Item
from pyasn1.error import PyAsn1Error
from pysnmp import debug

__all__ = ['ObjectIdentity', 'ObjectType', 'NotificationType']


class ObjectIdentity(object):
    """Create an object representing MIB variable ID.

    At the protocol level, MIB variable is only identified by an OID.
    However, when interacting with humans, MIB variable can also be referred
    to by its MIB name. The *ObjectIdentity* class supports various forms
    of MIB variable identification, providing automatic conversion from
    one to others. At the same time *ObjectIdentity* objects behave like
    :py:obj:`tuples` of py:obj:`int` sub-OIDs.

    See :RFC:`1902#section-2` for more information on OBJECT-IDENTITY
    SMI definitions.

    Parameters
    ----------
    args
        initial MIB variable identity. Recognized variants:

        * single :py:obj:`tuple` or integers representing OID
        * single :py:obj:`str` representing OID in dot-separated
          integers form
        * single :py:obj:`str` representing MIB variable in
          dot-separated labels form
        * single :py:obj:`str` representing MIB name. First variable
          defined in MIB is assumed.
        * pair of :py:obj:`str` representing MIB name and variable name
        * pair of :py:obj:`str` representing MIB name and variable name
          followed by an arbitrary number of :py:obj:`str` and/or
          :py:obj:`int` values representing MIB variable instance
          identification.

    Other parameters
    ----------------
    kwargs
        MIB resolution options(object):

        * whenever only MIB name is given, resolve into last variable defined
          in MIB if last=True.  Otherwise resolves to first variable (default).

    Notes
    -----
        Actual conversion between MIB variable representation formats occurs
        upon :py:meth:`~pysnmp.smi.rfc1902.ObjectIdentity.resolveWithMib`
        invocation.

    Examples
    --------
    >>> from pysnmp.smi.rfc1902 import ObjectIdentity
    >>> ObjectIdentity((1, 3, 6, 1, 2, 1, 1, 1, 0))
    ObjectIdentity((1, 3, 6, 1, 2, 1, 1, 1, 0))
    >>> ObjectIdentity('1.3.6.1.2.1.1.1.0')
    ObjectIdentity('1.3.6.1.2.1.1.1.0')
    >>> ObjectIdentity('iso.org.dod.internet.mgmt.mib-2.system.sysDescr.0')
    ObjectIdentity('iso.org.dod.internet.mgmt.mib-2.system.sysDescr.0')
    >>> ObjectIdentity('SNMPv2-MIB', 'system')
    ObjectIdentity('SNMPv2-MIB', 'system')
    >>> ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)
    ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)
    >>> ObjectIdentity('IP-MIB', 'ipAdEntAddr', '127.0.0.1', 123)
    ObjectIdentity('IP-MIB', 'ipAdEntAddr', '127.0.0.1', 123)

    """
    stDirty, stClean = 1, 2

    def __init__(self, *args, **kwargs):
        self.__args = args
        self.__kwargs = kwargs
        self.__mibSourcesToAdd = self.__modNamesToLoad = None
        self.__asn1SourcesToAdd = self.__asn1SourcesOptions = None
        self.__state = self.stDirty
        self.__indices = self.__oid = self.__label = ()
        self.__modName = self.__symName = ''
        self.__mibNode = None

    def getMibSymbol(self):
        """Returns MIB variable symbolic identification.

        Returns
        -------
        str
             MIB module name
        str
             MIB variable symbolic name
        : :py:class:`~pysnmp.proto.rfc1902.ObjectName`
             class instance representing MIB variable instance index.

        Raises
        ------
        SmiError
            If MIB variable conversion has not been performed.

        Examples
        --------
        >>> objectIdentity = ObjectIdentity('1.3.6.1.2.1.1.1.0')
        >>> objectIdentity.resolveWithMib(mibViewController)
        >>> objectIdentity.getMibSymbol()
        ('SNMPv2-MIB', 'sysDescr', (0,))
        >>>

        """
        if self.__state & self.stClean:
            return self.__modName, self.__symName, self.__indices
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)

    def getOid(self):
        """Returns OID identifying MIB variable.

        Returns
        -------
        : :py:class:`~pysnmp.proto.rfc1902.ObjectName`
            full OID identifying MIB variable including possible index part.

        Raises
        ------
        SmiError
           If MIB variable conversion has not been performed.

        Examples
        --------
        >>> objectIdentity = ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)
        >>> objectIdentity.resolveWithMib(mibViewController)
        >>> objectIdentity.getOid()
        ObjectName('1.3.6.1.2.1.1.1.0')
        >>>

        """
        if self.__state & self.stClean:
            return self.__oid
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)

    def getLabel(self):
        """Returns symbolic path to this MIB variable.

        Meaning a sequence of symbolic identifications for each of parent
        MIB objects in MIB tree.

        Returns
        -------
        tuple
            sequence of names of nodes in a MIB tree from the top of the tree
            towards this MIB variable.

        Raises
        ------
        SmiError
           If MIB variable conversion has not been performed.

        Notes
        -----
        Returned sequence may not contain full path to this MIB variable
        if some symbols are now known at the moment of MIB look up.

        Examples
        --------
        >>> objectIdentity = ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)
        >>> objectIdentity.resolveWithMib(mibViewController)
        >>> objectIdentity.getOid()
        ('iso', 'org', 'dod', 'internet', 'mgmt', 'mib-2', 'system', 'sysDescr')
        >>>

        """
        if self.__state & self.stClean:
            return self.__label
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)

    def getMibNode(self):
        if self.__state & self.stClean:
            return self.__mibNode
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)

    def isFullyResolved(self):
        return self.__state & self.stClean

    #
    # A gateway to MIBs manipulation routines
    #

    def addAsn1MibSource(self, *asn1Sources, **kwargs):
        """Adds path to a repository to search ASN.1 MIB files.

        Parameters
        ----------
        *asn1Sources :
            one or more URL in form of :py:obj:`str` identifying local or
            remote ASN.1 MIB repositories. Path must include the *@mib@*
            component which will be replaced with MIB module name at the
            time of search.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
            reference to itself

        Notes
        -----
        Please refer to :py:class:`~pysmi.reader.localfile.FileReader`,
        :py:class:`~pysmi.reader.httpclient.HttpReader` and
        :py:class:`~pysmi.reader.ftpclient.FtpReader` classes for
        in-depth information on ASN.1 MIB lookup.

        Examples
        --------
        >>> ObjectIdentity('SNMPv2-MIB', 'sysDescr').addAsn1Source('http://mibs.snmplabs.com/asn1/@mib@')
        ObjectIdentity('SNMPv2-MIB', 'sysDescr')
        >>>

        """
        if self.__asn1SourcesToAdd is None:
            self.__asn1SourcesToAdd = asn1Sources
        else:
            self.__asn1SourcesToAdd += asn1Sources
        if self.__asn1SourcesOptions:
            self.__asn1SourcesOptions.update(kwargs)
        else:
            self.__asn1SourcesOptions = kwargs
        return self

    def addMibSource(self, *mibSources):
        """Adds path to repository to search PySNMP MIB files.

        Parameters
        ----------
        *mibSources :
            one or more paths to search or Python package names to import
            and search for PySNMP MIB modules.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
            reference to itself

        Notes
        -----
        Normally, ASN.1-to-Python MIB modules conversion is performed
        automatically through PySNMP/PySMI interaction. ASN1 MIB modules
        could also be manually compiled into Python via the
        `mibdump.py <http://pysmi.sourceforge.net/user-perspective.html>`_
        tool.

        Examples
        --------
        >>> ObjectIdentity('SNMPv2-MIB', 'sysDescr').addMibSource('/opt/pysnmp/mibs', 'pysnmp_mibs')
        ObjectIdentity('SNMPv2-MIB', 'sysDescr')
        >>>

        """
        if self.__mibSourcesToAdd is None:
            self.__mibSourcesToAdd = mibSources
        else:
            self.__mibSourcesToAdd += mibSources
        return self

    # provides deferred MIBs load
    def loadMibs(self, *modNames):
        """Schedules search and load of given MIB modules.

        Parameters
        ----------
        *modNames:
            one or more MIB module names to load up and use for MIB
            variables resolution purposes.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
            reference to itself

        Examples
        --------
        >>> ObjectIdentity('SNMPv2-MIB', 'sysDescr').loadMibs('IF-MIB', 'TCP-MIB')
        ObjectIdentity('SNMPv2-MIB', 'sysDescr')
        >>>

        """
        if self.__modNamesToLoad is None:
            self.__modNamesToLoad = modNames
        else:
            self.__modNamesToLoad += modNames
        return self

    # this would eventually be called by an entity which posses a
    # reference to MibViewController
    def resolveWithMib(self, mibViewController):
        """Perform MIB variable ID conversion.

        Parameters
        ----------
        mibViewController : :py:class:`~pysnmp.smi.view.MibViewController`
            class instance representing MIB browsing functionality.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
            reference to itself

        Raises
        ------
        SmiError
           In case of fatal MIB hanling errora

        Notes
        -----
        Calling this method might cause the following sequence of
        events (exact details depends on many factors):

        * ASN.1 MIB file downloaded and handed over to
          :py:class:`~pysmi.compiler.MibCompiler` for conversion into
          Python MIB module (based on pysnmp classes)
        * Python MIB module is imported by SNMP engine, internal indices
          created
        * :py:class:`~pysnmp.smi.view.MibViewController` looks up the
          rest of MIB identification information based on whatever information
          is already available, :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
          class instance
          gets updated and ready for further use.

        Examples
        --------
        >>> objectIdentity = ObjectIdentity('SNMPv2-MIB', 'sysDescr')
        >>> objectIdentity.resolveWithMib(mibViewController)
        ObjectIdentity('SNMPv2-MIB', 'sysDescr')
        >>>

        """
        if self.__mibSourcesToAdd is not None:
            debug.logger & debug.flagMIB and debug.logger('adding MIB sources %s' % ', '.join(self.__mibSourcesToAdd))
            mibViewController.mibBuilder.addMibSources(
                *[ZipMibSource(x) for x in self.__mibSourcesToAdd]
            )
            self.__mibSourcesToAdd = None

        if self.__asn1SourcesToAdd is None:
            addMibCompiler(mibViewController.mibBuilder,
                           ifAvailable=True, ifNotAdded=True)
        else:
            debug.logger & debug.flagMIB and debug.logger(
                'adding MIB compiler with source paths %s' % ', '.join(self.__asn1SourcesToAdd))
            addMibCompiler(
                mibViewController.mibBuilder,
                sources=self.__asn1SourcesToAdd,
                searchers=self.__asn1SourcesOptions.get('searchers'),
                borrowers=self.__asn1SourcesOptions.get('borrowers'),
                destination=self.__asn1SourcesOptions.get('destination'),
                ifAvailable=self.__asn1SourcesOptions.get('ifAvailable'),
                ifNotAdded=self.__asn1SourcesOptions.get('ifNotAdded')
            )
            self.__asn1SourcesToAdd = self.__asn1SourcesOptions = None

        if self.__modNamesToLoad is not None:
            debug.logger & debug.flagMIB and debug.logger('loading MIB modules %s' % ', '.join(self.__modNamesToLoad))
            mibViewController.mibBuilder.loadModules(*self.__modNamesToLoad)
            self.__modNamesToLoad = None

        if self.__state & self.stClean:
            return self

        MibScalar, MibTableColumn = mibViewController.mibBuilder.importSymbols('SNMPv2-SMI', 'MibScalar',
                                                                               'MibTableColumn')

        self.__indices = ()

        if isinstance(self.__args[0], ObjectIdentity):
            self.__args[0].resolveWithMib(mibViewController)

        if len(self.__args) == 1:  # OID or label or MIB module
            debug.logger & debug.flagMIB and debug.logger('resolving %s as OID or label' % self.__args)
            try:
                # pyasn1 ObjectIdentifier or sequence of ints or string OID
                self.__oid = rfc1902.ObjectName(self.__args[0])  # OID
            except PyAsn1Error:
                # sequence of sub-OIDs and labels
                if isinstance(self.__args[0], (list, tuple)):
                    prefix, label, suffix = mibViewController.getNodeName(
                        self.__args[0]
                    )
                # string label
                elif '.' in self.__args[0]:
                    prefix, label, suffix = mibViewController.getNodeNameByOid(
                        tuple(self.__args[0].split('.'))
                    )
                # MIB module name
                else:
                    modName = self.__args[0]
                    mibViewController.mibBuilder.loadModules(modName)
                    if self.__kwargs.get('last'):
                        prefix, label, suffix = mibViewController.getLastNodeName(modName)
                    else:
                        prefix, label, suffix = mibViewController.getFirstNodeName(modName)

                if suffix:
                    try:
                        suffix = tuple([int(x) for x in suffix])
                    except ValueError:
                        raise SmiError('Unknown object name component %r' % (suffix,))
                self.__oid = rfc1902.ObjectName(prefix + suffix)
            else:
                prefix, label, suffix = mibViewController.getNodeNameByOid(
                    self.__oid
                )

            debug.logger & debug.flagMIB and debug.logger(
                'resolved %r into prefix %r and suffix %r' % (self.__args, prefix, suffix))

            modName, symName, _ = mibViewController.getNodeLocation(prefix)

            self.__modName = modName
            self.__symName = symName

            self.__label = label

            mibNode, = mibViewController.mibBuilder.importSymbols(
                modName, symName
            )

            self.__mibNode = mibNode

            debug.logger & debug.flagMIB and debug.logger('resolved prefix %r into MIB node %r' % (prefix, mibNode))

            if isinstance(mibNode, MibTableColumn):  # table column
                if suffix:
                    rowModName, rowSymName, _ = mibViewController.getNodeLocation(
                        mibNode.name[:-1]
                    )
                    rowNode, = mibViewController.mibBuilder.importSymbols(
                        rowModName, rowSymName
                    )
                    self.__indices = rowNode.getIndicesFromInstId(suffix)
            elif isinstance(mibNode, MibScalar):  # scalar
                if suffix:
                    self.__indices = (rfc1902.ObjectName(suffix),)
            else:
                if suffix:
                    self.__indices = (rfc1902.ObjectName(suffix),)
            self.__state |= self.stClean

            debug.logger & debug.flagMIB and debug.logger('resolved indices are %r' % (self.__indices,))

            return self
        elif len(self.__args) > 1:  # MIB, symbol[, index, index ...]
            # MIB, symbol, index, index
            if self.__args[0] and self.__args[1]:
                self.__modName = self.__args[0]
                self.__symName = self.__args[1]
            # MIB, ''
            elif self.__args[0]:
                mibViewController.mibBuilder.loadModules(self.__args[0])
                if self.__kwargs.get('last'):
                    prefix, label, suffix = mibViewController.getLastNodeName(self.__args[0])
                else:
                    prefix, label, suffix = mibViewController.getFirstNodeName(self.__args[0])
                self.__modName, self.__symName, _ = mibViewController.getNodeLocation(prefix)
            # '', symbol, index, index
            else:
                prefix, label, suffix = mibViewController.getNodeName(self.__args[1:])
                self.__modName, self.__symName, _ = mibViewController.getNodeLocation(prefix)

            mibNode, = mibViewController.mibBuilder.importSymbols(
                self.__modName, self.__symName
            )

            self.__mibNode = mibNode

            self.__oid = rfc1902.ObjectName(mibNode.getName())

            prefix, label, suffix = mibViewController.getNodeNameByOid(
                self.__oid
            )
            self.__label = label

            debug.logger & debug.flagMIB and debug.logger(
                'resolved %r into prefix %r and suffix %r' % (self.__args, prefix, suffix))

            if isinstance(mibNode, MibTableColumn):  # table
                rowModName, rowSymName, _ = mibViewController.getNodeLocation(
                    mibNode.name[:-1]
                )
                rowNode, = mibViewController.mibBuilder.importSymbols(
                    rowModName, rowSymName
                )
                if self.__args[2:]:
                    try:
                        instIds = rowNode.getInstIdFromIndices(*self.__args[2:])
                        self.__oid += instIds
                        self.__indices = rowNode.getIndicesFromInstId(instIds)
                    except PyAsn1Error:
                        raise SmiError('Instance index %r to OID convertion failure at object %r: %s' % (
                            self.__args[2:], mibNode.getLabel(), sys.exc_info()[1]))
            elif self.__args[2:]:  # any other kind of MIB node with indices
                if self.__args[2:]:
                    instId = rfc1902.ObjectName(
                        '.'.join([str(x) for x in self.__args[2:]])
                    )
                    self.__oid += instId
                    self.__indices = (instId,)
            self.__state |= self.stClean

            debug.logger & debug.flagMIB and debug.logger('resolved indices are %r' % (self.__indices,))

            return self
        else:
            raise SmiError('Non-OID, label or MIB symbol')

    def prettyPrint(self):
        if self.__state & self.stClean:
            s = rfc1902.OctetString()
            return '%s::%s%s%s' % (
                self.__modName, self.__symName,
                self.__indices and '.' or '',
                '.'.join([x.isSuperTypeOf(s) and '"%s"' % x.prettyPrint() or x.prettyPrint() for x in self.__indices])
            )
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)

    def __repr__(self):
        return '%s(%s)' % (self.__class__.__name__, ', '.join([repr(x) for x in self.__args]))

    # Redirect some attrs access to the OID object to behave alike

    def __str__(self):
        if self.__state & self.stClean:
            return str(self.__oid)
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __eq__(self, other):
        if self.__state & self.stClean:
            return self.__oid == other
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __ne__(self, other):
        if self.__state & self.stClean:
            return self.__oid != other
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __lt__(self, other):
        if self.__state & self.stClean:
            return self.__oid < other
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __le__(self, other):
        if self.__state & self.stClean:
            return self.__oid <= other
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __gt__(self, other):
        if self.__state & self.stClean:
            return self.__oid > other
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __ge__(self, other):
        if self.__state & self.stClean:
            return self.__oid > other
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __nonzero__(self):
        if self.__state & self.stClean:
            return self.__oid != 0
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __bool__(self):
        if self.__state & self.stClean:
            return bool(self.__oid)
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __getitem__(self, i):
        if self.__state & self.stClean:
            return self.__oid[i]
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __len__(self):
        if self.__state & self.stClean:
            return len(self.__oid)
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __add__(self, other):
        if self.__state & self.stClean:
            return self.__oid + other
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __radd__(self, other):
        if self.__state & self.stClean:
            return other + self.__oid
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __hash__(self):
        if self.__state & self.stClean:
            return hash(self.__oid)
        else:
            raise SmiError('%s object not properly initialized' % self.__class__.__name__)

    def __getattr__(self, attr):
        if self.__state & self.stClean:
            if attr in ('asTuple', 'clone', 'subtype', 'isPrefixOf',
                        'isSameTypeWith', 'isSuperTypeOf', 'getTagSet',
                        'getEffectiveTagSet', 'getTagMap', 'tagSet', 'index'):
                return getattr(self.__oid, attr)
            raise AttributeError(attr)
        else:
            raise SmiError('%s object not properly initialized for accessing %s' % (self.__class__.__name__, attr))


# A two-element sequence of ObjectIdentity and SNMP data type object
class ObjectType(object):
    """Create an object representing MIB variable.

    Instances of :py:class:`~pysnmp.smi.rfc1902.ObjectType` class are
    containers incorporating :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
    class instance (identifying MIB variable) and optional value belonging
    to one of SNMP types (:RFC:`1902`).

    Typical MIB variable is defined like this (from *SNMPv2-MIB.txt*):

    .. code-block:: bash

       sysDescr OBJECT-TYPE
           SYNTAX      DisplayString (SIZE (0..255))
           MAX-ACCESS  read-only
           STATUS      current
           DESCRIPTION
                   "A textual description of the entity.  This value should..."
           ::= { system 1 }

    Corresponding ObjectType instantiation would look like this:

    .. code-block:: python

        ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr'), 'Linux i386 box')

    In order to behave like SNMP variable-binding (:RFC:`1157#section-4.1.1`),
    :py:class:`~pysnmp.smi.rfc1902.ObjectType` objects also support
    sequence protocol addressing `objectIdentity` as its 0-th element
    and `objectSyntax` as 1-st.

    See :RFC:`1902#section-2` for more information on OBJECT-TYPE SMI
    definitions.

    Parameters
    ----------
    objectIdentity : :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
        Class instance representing MIB variable identification.
    objectSyntax :
        Represents a value associated with this MIB variable. Values of
        built-in Python types will be automatically converted into SNMP
        object as specified in OBJECT-TYPE->SYNTAX field.

    Notes
    -----
        Actual conversion between MIB variable representation formats occurs
        upon :py:meth:`~pysnmp.smi.rfc1902.ObjectType.resolveWithMib`
        invocation.

    Examples
    --------
    >>> from pysnmp.smi.rfc1902 import *
    >>> ObjectType(ObjectIdentity('1.3.6.1.2.1.1.1.0'))
    ObjectType(ObjectIdentity('1.3.6.1.2.1.1.1.0'), Null())
    >>> ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0), 'Linux i386')
    ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0), 'Linux i386')

    """
    stDirty, stClean = 1, 2

    def __init__(self, objectIdentity, objectSyntax=rfc1905.unSpecified):
        if not isinstance(objectIdentity, ObjectIdentity):
            raise SmiError('initializer should be ObjectIdentity instance, not %r' % (objectIdentity,))
        self.__args = [objectIdentity, objectSyntax]
        self.__state = self.stDirty

    def __getitem__(self, i):
        if self.__state & self.stClean:
            return self.__args[i]
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)

    def __str__(self):
        return self.prettyPrint()

    def __repr__(self):
        return '%s(%s)' % (self.__class__.__name__, ', '.join([repr(x) for x in self.__args]))

    def isFullyResolved(self):
        return self.__state & self.stClean

    def addAsn1MibSource(self, *asn1Sources, **kwargs):
        """Adds path to a repository to search ASN.1 MIB files.

        Parameters
        ----------
        *asn1Sources :
            one or more URL in form of :py:obj:`str` identifying local or
            remote ASN.1 MIB repositories. Path must include the *@mib@*
            component which will be replaced with MIB module name at the
            time of search.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
            reference to itself

        Notes
        -----
        Please refer to :py:class:`~pysmi.reader.localfile.FileReader`,
        :py:class:`~pysmi.reader.httpclient.HttpReader` and
        :py:class:`~pysmi.reader.ftpclient.FtpReader` classes for
        in-depth information on ASN.1 MIB lookup.

        Examples
        --------
        >>> ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr')).addAsn1Source('http://mibs.snmplabs.com/asn1/@mib@')
        ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr'))
        >>>

        """
        self.__args[0].addAsn1MibSource(*asn1Sources, **kwargs)
        return self

    def addMibSource(self, *mibSources):
        """Adds path to repository to search PySNMP MIB files.

        Parameters
        ----------
        *mibSources :
            one or more paths to search or Python package names to import
            and search for PySNMP MIB modules.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
            reference to itself

        Notes
        -----
        Normally, ASN.1-to-Python MIB modules conversion is performed
        automatically through PySNMP/PySMI interaction. ASN1 MIB modules
        could also be manually compiled into Python via the
        `mibdump.py <http://pysmi.sourceforge.net/user-perspective.html>`_
        tool.

        Examples
        --------
        >>> ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr')).addMibSource('/opt/pysnmp/mibs', 'pysnmp_mibs')
        ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr'))
        >>>

        """
        self.__args[0].addMibSource(*mibSources)
        return self

    def loadMibs(self, *modNames):
        """Schedules search and load of given MIB modules.

        Parameters
        ----------
        *modNames:
            one or more MIB module names to load up and use for MIB
            variables resolution purposes.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
            reference to itself

        Examples
        --------
        >>> ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr')).loadMibs('IF-MIB', 'TCP-MIB')
        ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr'))
        >>>

        """
        self.__args[0].loadMibs(*modNames)
        return self

    def resolveWithMib(self, mibViewController):
        """Perform MIB variable ID and associated value conversion.

        Parameters
        ----------
        mibViewController : :py:class:`~pysnmp.smi.view.MibViewController`
            class instance representing MIB browsing functionality.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
            reference to itself

        Raises
        ------
        SmiError
           In case of fatal MIB hanling errora

        Notes
        -----
        Calling this method involves
        :py:meth:`~pysnmp.smi.rfc1902.ObjectIdentity.resolveWithMib`
        method invocation.

        Examples
        --------
        >>> objectType = ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr'), 'Linux i386')
        >>> objectType.resolveWithMib(mibViewController)
        ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr'), DisplayString('Linux i386'))
        >>> str(objectType)
        'SNMPv2-MIB::sysDescr."0" = Linux i386'
        >>>

        """
        if self.__state & self.stClean:
            return self

        self.__args[0].resolveWithMib(mibViewController)

        MibScalar, MibTableColumn = mibViewController.mibBuilder.importSymbols('SNMPv2-SMI', 'MibScalar',
                                                                               'MibTableColumn')

        if not isinstance(self.__args[0].getMibNode(),
                          (MibScalar, MibTableColumn)):
            if not isinstance(self.__args[1], AbstractSimpleAsn1Item):
                raise SmiError('MIB object %r is not OBJECT-TYPE (MIB not loaded?)' % (self.__args[0],))
            self.__state |= self.stClean
            return self

        if isinstance(self.__args[1], (rfc1905.UnSpecified,
                                       rfc1905.NoSuchObject,
                                       rfc1905.NoSuchInstance,
                                       rfc1905.EndOfMibView)):
            self.__state |= self.stClean
            return self

        try:
            self.__args[1] = self.__args[0].getMibNode().getSyntax().clone(self.__args[1])
        except PyAsn1Error:
            raise SmiError('MIB object %r having type %r failed to cast value %r: %s' % (
                self.__args[0].prettyPrint(), self.__args[0].getMibNode().getSyntax().__class__.__name__, self.__args[1],
                sys.exc_info()[1]))

        if self.__args[1].isSuperTypeOf(rfc1902.ObjectIdentifier()):
            self.__args[1] = ObjectIdentity(self.__args[1]).resolveWithMib(mibViewController)

        self.__state |= self.stClean

        debug.logger & debug.flagMIB and debug.logger('resolved %r syntax is %r' % (self.__args[0], self.__args[1]))

        return self

    def prettyPrint(self):
        if self.__state & self.stClean:
            return '%s = %s' % (self.__args[0].prettyPrint(),
                                self.__args[1].prettyPrint())
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)


class NotificationType(object):
    """Create an object representing SNMP Notification.

    Instances of :py:class:`~pysnmp.smi.rfc1902.NotificationType` class are
    containers incorporating :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
    class instance (identifying particular notification) and a collection
    of MIB variables IDs that
    :py:class:`~pysnmp.entity.rfc3413.oneliner.cmdgen.NotificationOriginator`
    should gather and put into notification message.

    Typical notification is defined like this (from *IF-MIB.txt*):

    .. code-block:: bash

       linkDown NOTIFICATION-TYPE
           OBJECTS { ifIndex, ifAdminStatus, ifOperStatus }
           STATUS  current
           DESCRIPTION
                  "A linkDown trap signifies that the SNMP entity..."
           ::= { snmpTraps 3 }

    Corresponding NotificationType instantiation would look like this:

    .. code-block:: python

        NotificationType(ObjectIdentity('IF-MIB', 'linkDown'))

    To retain similarity with SNMP variable-bindings,
    :py:class:`~pysnmp.smi.rfc1902.NotificationType` objects behave like
    a sequence of :py:class:`~pysnmp.smi.rfc1902.ObjectType` class
    instances.

    See :RFC:`1902#section-2` for more information on NOTIFICATION-TYPE SMI
    definitions.

    Parameters
    ----------
    objectIdentity : :py:class:`~pysnmp.smi.rfc1902.ObjectIdentity`
        Class instance representing MIB notification type identification.
    instanceIndex : :py:class:`~pysnmp.proto.rfc1902.ObjectName`
        Trailing part of MIB variables OID identification that represents
        concrete instance of a MIB variable. When notification is prepared,
        `instanceIndex` is appended to each MIB variable identification
        listed in NOTIFICATION-TYPE->OBJECTS clause.
    objects : dict
        Dictionary-like object that may return values by OID key. The
        `objects` dictionary is consulted when notification is being
        prepared. OIDs are taken from MIB variables listed in
        NOTIFICATION-TYPE->OBJECTS with `instanceIndex` part appended.

    Notes
    -----
        Actual notification type and MIB variables look up occurs
        upon :py:meth:`~pysnmp.smi.rfc1902.NotificationType.resolveWithMib`
        invocation.

    Examples
    --------
    >>> from pysnmp.smi.rfc1902 import *
    >>> NotificationType(ObjectIdentity('1.3.6.1.6.3.1.1.5.3'))
    NotificationType(ObjectIdentity('1.3.6.1.6.3.1.1.5.3'), (), {})
    >>> NotificationType(ObjectIdentity('IP-MIB', 'linkDown'), ObjectName('3.5'))
    NotificationType(ObjectIdentity('1.3.6.1.6.3.1.1.5.3'), ObjectName('3.5'), {})

    """
    stDirty, stClean = 1, 2

    def __init__(self, objectIdentity, instanceIndex=(), objects={}):
        if not isinstance(objectIdentity, ObjectIdentity):
            raise SmiError('initializer should be ObjectIdentity instance, not %r' % (objectIdentity,))
        self.__objectIdentity = objectIdentity
        self.__instanceIndex = instanceIndex
        self.__objects = objects
        self.__varBinds = []
        self.__additionalVarBinds = []
        self.__state = self.stDirty

    def __getitem__(self, i):
        if self.__state & self.stClean:
            return self.__varBinds[i]
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)

    def __repr__(self):
        return '%s(%r, %r, %r)' % (self.__class__.__name__, self.__objectIdentity, self.__instanceIndex, self.__objects)

    def addVarBinds(self, *varBinds):
        """Appends variable-binding to notification.

        Parameters
        ----------
        *varBinds : :py:class:`~pysnmp.smi.rfc1902.ObjectType`
            One or more :py:class:`~pysnmp.smi.rfc1902.ObjectType` class
            instances.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.NotificationType`
            reference to itself

        Notes
        -----
        This method can be used to add custom variable-bindings to
        notification message in addition to MIB variables specified
        in NOTIFICATION-TYPE->OBJECTS clause.

        Examples
        --------
        >>> nt = NotificationType(ObjectIdentity('IP-MIB', 'linkDown'))
        >>> nt.addVarBinds(ObjectType(ObjectIdentity('SNMPv2-MIB', 'sysDescr', 0)))
        NotificationType(ObjectIdentity('IP-MIB', 'linkDown'), (), {})
        >>>

        """
        debug.logger & debug.flagMIB and debug.logger('additional var-binds: %r' % (varBinds,))
        if self.__state & self.stClean:
            raise SmiError('%s object is already sealed' % self.__class__.__name__)
        else:
            self.__additionalVarBinds.extend(varBinds)
        return self

    def addAsn1MibSource(self, *asn1Sources, **kwargs):
        """Adds path to a repository to search ASN.1 MIB files.

        Parameters
        ----------
        *asn1Sources :
            one or more URL in form of :py:obj:`str` identifying local or
            remote ASN.1 MIB repositories. Path must include the *@mib@*
            component which will be replaced with MIB module name at the
            time of search.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.NotificationType`
            reference to itself

        Notes
        -----
        Please refer to :py:class:`~pysmi.reader.localfile.FileReader`,
        :py:class:`~pysmi.reader.httpclient.HttpReader` and
        :py:class:`~pysmi.reader.ftpclient.FtpReader` classes for
        in-depth information on ASN.1 MIB lookup.

        Examples
        --------
        >>> NotificationType(ObjectIdentity('IF-MIB', 'linkDown'), (), {}).addAsn1Source('http://mibs.snmplabs.com/asn1/@mib@')
        NotificationType(ObjectIdentity('IF-MIB', 'linkDown'), (), {})
        >>>

        """
        self.__objectIdentity.addAsn1MibSource(*asn1Sources, **kwargs)
        return self

    def addMibSource(self, *mibSources):
        """Adds path to repository to search PySNMP MIB files.

        Parameters
        ----------
        *mibSources :
            one or more paths to search or Python package names to import
            and search for PySNMP MIB modules.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.NotificationType`
            reference to itself

        Notes
        -----
        Normally, ASN.1-to-Python MIB modules conversion is performed
        automatically through PySNMP/PySMI interaction. ASN1 MIB modules
        could also be manually compiled into Python via the
        `mibdump.py <http://pysmi.sourceforge.net/user-perspective.html>`_
        tool.

        Examples
        --------
        >>> NotificationType(ObjectIdentity('IF-MIB', 'linkDown'), (), {}).addMibSource('/opt/pysnmp/mibs', 'pysnmp_mibs')
        NotificationType(ObjectIdentity('IF-MIB', 'linkDown'), (), {})
        >>>

        """
        self.__objectIdentity.addMibSource(*mibSources)
        return self

    def loadMibs(self, *modNames):
        """Schedules search and load of given MIB modules.

        Parameters
        ----------
        *modNames:
            one or more MIB module names to load up and use for MIB
            variables resolution purposes.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.NotificationType`
            reference to itself

        Examples
        --------
        >>> NotificationType(ObjectIdentity('IF-MIB', 'linkDown'), (), {}).loadMibs('IF-MIB', 'TCP-MIB')
        NotificationType(ObjectIdentity('IF-MIB', 'linkDown'), (), {})
        >>>

        """
        self.__objectIdentity.loadMibs(*modNames)
        return self

    def isFullyResolved(self):
        return self.__state & self.stClean

    def resolveWithMib(self, mibViewController):
        """Perform MIB variable ID conversion and notification objects expansion.

        Parameters
        ----------
        mibViewController : :py:class:`~pysnmp.smi.view.MibViewController`
            class instance representing MIB browsing functionality.

        Returns
        -------
        : :py:class:`~pysnmp.smi.rfc1902.NotificationType`
            reference to itself

        Raises
        ------
        SmiError
           In case of fatal MIB hanling errora

        Notes
        -----
        Calling this method might cause the following sequence of
        events (exact details depends on many factors):

        * :py:meth:`pysnmp.smi.rfc1902.ObjectIdentity.resolveWithMib` is called
        * MIB variables names are read from NOTIFICATION-TYPE->OBJECTS clause,
          :py:class:`~pysnmp.smi.rfc1902.ObjectType` instances are created
          from MIB variable OID and `indexInstance` suffix.
        * `objects` dictionary is queried for each MIB variable OID,
          acquired values are added to corresponding MIB variable

        Examples
        --------
        >>> notificationType = NotificationType(ObjectIdentity('IF-MIB', 'linkDown'))
        >>> notificationType.resolveWithMib(mibViewController)
        NotificationType(ObjectIdentity('IF-MIB', 'linkDown'), (), {})
        >>>

        """
        if self.__state & self.stClean:
            return self

        self.__objectIdentity.resolveWithMib(mibViewController)

        self.__varBinds.append(
            ObjectType(ObjectIdentity(v2c.apiTrapPDU.snmpTrapOID),
                       self.__objectIdentity).resolveWithMib(mibViewController)
        )

        SmiNotificationType, = mibViewController.mibBuilder.importSymbols('SNMPv2-SMI', 'NotificationType')

        mibNode = self.__objectIdentity.getMibNode()

        varBindsLocation = {}

        if isinstance(mibNode, SmiNotificationType):
            for notificationObject in mibNode.getObjects():
                objectIdentity = ObjectIdentity(*notificationObject + self.__instanceIndex).resolveWithMib(
                    mibViewController)
                self.__varBinds.append(
                    ObjectType(objectIdentity,
                               self.__objects.get(notificationObject, rfc1905.unSpecified)).resolveWithMib(
                        mibViewController)
                )
                varBindsLocation[objectIdentity] = len(self.__varBinds) - 1
        else:
            debug.logger & debug.flagMIB and debug.logger(
                'WARNING: MIB object %r is not NOTIFICATION-TYPE (MIB not loaded?)' % (self.__objectIdentity,))

        for varBinds in self.__additionalVarBinds:
            if not isinstance(varBinds, ObjectType):
                varBinds = ObjectType(ObjectIdentity(varBinds[0]), varBinds[1])
            varBinds.resolveWithMib(mibViewController)
            if varBinds[0] in varBindsLocation:
                self.__varBinds[varBindsLocation[varBinds[0]]] = varBinds
            else:
                self.__varBinds.append(varBinds)

        self.__additionalVarBinds = []

        self.__state |= self.stClean

        debug.logger & debug.flagMIB and debug.logger('resolved %r into %r' % (self.__objectIdentity, self.__varBinds))

        return self

    def prettyPrint(self):
        if self.__state & self.stClean:
            return ' '.join(['%s = %s' % (x[0].prettyPrint(), x[1].prettyPrint()) for x in self.__varBinds])
        else:
            raise SmiError('%s object not fully initialized' % self.__class__.__name__)
