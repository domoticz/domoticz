#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# THESE FUNCTIONS ARE OBSOLETE AND MUST NOT BE USED!
# USE pysnmp.entity.rfc3413.oneliner.mibvar INSTEAD
#
from pyasn1.type import univ
from pysnmp.smi.error import NoSuchObjectError


# Name

def mibNameToOid(mibView, name):
    if isinstance(name[0], tuple):
        f = lambda x='', y='': (x, y)
        modName, symName = f(*name[0])
        if modName:  # load module if needed
            mibView.mibBuilder.loadModules(modName)
        else:
            mibView.mibBuilder.loadModules()  # load all (slow)
        if symName:
            oid, label, suffix = mibView.getNodeNameByDesc(symName, modName)
        else:
            oid, label, suffix = mibView.getFirstNodeName(modName)
        suffix = name[1:]
        modName, symName, _s = mibView.getNodeLocation(oid)
        mibNode, = mibView.mibBuilder.importSymbols(
            modName, symName
        )
        if hasattr(mibNode, 'createTest'):  # table column XXX
            modName, symName, _s = mibView.getNodeLocation(oid[:-1])
            rowNode, = mibView.mibBuilder.importSymbols(modName, symName)
            return oid, rowNode.getInstIdFromIndices(*suffix)
        else:  # scalar or incomplete spec
            return oid, suffix
    elif not isinstance(name, tuple):
        name = tuple(univ.ObjectIdentifier(name))

    oid, label, suffix = mibView.getNodeNameByOid(name)

    return oid, suffix


__scalarSuffix = (univ.Integer(0),)


def oidToMibName(mibView, oid):
    if not isinstance(oid, tuple):
        oid = tuple(univ.ObjectIdentifier(oid))
    _oid, label, suffix = mibView.getNodeNameByOid(oid)
    modName, symName, __suffix = mibView.getNodeLocation(_oid)
    mibNode, = mibView.mibBuilder.importSymbols(
        modName, symName
    )
    if hasattr(mibNode, 'createTest'):  # table column
        __modName, __symName, __s = mibView.getNodeLocation(_oid[:-1])
        rowNode, = mibView.mibBuilder.importSymbols(__modName, __symName)
        return (symName, modName), rowNode.getIndicesFromInstId(suffix)
    elif not suffix:  # scalar
        return (symName, modName), suffix
    elif suffix == (0,):  # scalar
        return (symName, modName), __scalarSuffix
    else:
        raise NoSuchObjectError(
            str='No MIB registered that defines %s object, closest known parent is %s (%s::%s)' % (
                univ.ObjectIdentifier(oid), univ.ObjectIdentifier(mibNode.name), modName, symName)
        )


# Value

def cloneFromMibValue(mibView, modName, symName, value):
    mibNode, = mibView.mibBuilder.importSymbols(
        modName, symName
    )
    if hasattr(mibNode, 'syntax'):  # scalar
        return mibNode.syntax.clone(value)
    else:
        return  # identifier
