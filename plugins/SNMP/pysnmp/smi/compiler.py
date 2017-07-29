#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import os
import sys

defaultSources = ['file:///usr/share/snmp/mibs', 'file:///usr/share/mibs']

if sys.platform[:3] == 'win':
    defaultDest = os.path.join(os.path.expanduser("~"),
                               'PySNMP Configuration', 'mibs')
else:
    defaultDest = os.path.join(os.path.expanduser("~"), '.pysnmp', 'mibs')

defaultBorrowers = []

try:
    from pysmi.reader.url import getReadersFromUrls
    from pysmi.searcher.pypackage import PyPackageSearcher
    from pysmi.searcher.stub import StubSearcher
    from pysmi.borrower.pyfile import PyFileBorrower
    from pysmi.writer.pyfile import PyFileWriter
    from pysmi.parser.smi import parserFactory
    from pysmi.parser.dialect import smiV1Relaxed
    from pysmi.codegen.pysnmp import PySnmpCodeGen, baseMibs
    from pysmi.compiler import MibCompiler

except ImportError:
    from pysnmp.smi import error


    def addMibCompilerDecorator(errorMsg):
        def addMibCompiler(mibBuilder, **kwargs):
            if not kwargs.get('ifAvailable'):
                raise error.SmiError('MIB compiler not available: %s' % errorMsg)

        return addMibCompiler


    addMibCompiler = addMibCompilerDecorator(sys.exc_info()[1])

else:

    def addMibCompiler(mibBuilder, **kwargs):
        if kwargs.get('ifNotAdded') and mibBuilder.getMibCompiler():
            return

        compiler = MibCompiler(parserFactory(**smiV1Relaxed)(),
                               PySnmpCodeGen(),
                               PyFileWriter(kwargs.get('destination') or defaultDest))

        compiler.addSources(*getReadersFromUrls(*kwargs.get('sources') or defaultSources))

        compiler.addSearchers(StubSearcher(*baseMibs))
        compiler.addSearchers(*[PyPackageSearcher(x.fullPath()) for x in mibBuilder.getMibSources()])
        compiler.addBorrowers(*[PyFileBorrower(x, genTexts=mibBuilder.loadTexts) for x in
                                getReadersFromUrls(*kwargs.get('borrowers') or defaultBorrowers,
                                                   **dict(lowcaseMatching=False))])

        mibBuilder.setMibCompiler(
            compiler, kwargs.get('destination') or defaultDest
        )
