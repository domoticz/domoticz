#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto.secmod.eso.priv import aesbase


class AesBlumenthal256(aesbase.AbstractAesBlumenthal):
    """AES 256 bit encryption (Internet draft)

       http://tools.ietf.org/html/draft-blumenthal-aes-usm-04
    """
    serviceID = (1, 3, 6, 1, 4, 1, 9, 12, 6, 1, 2)  # cusmAESCfb256PrivProtocol
    keySize = 32


class Aes256(aesbase.AbstractAesReeder):
    """AES 256 bit encryption (Internet draft)

    Reeder AES encryption with non-standard key localization algorithm
    borrowed from Reeder 3DES draft:

    http://tools.ietf.org/html/draft-blumenthal-aes-usm-04
    https://tools.ietf.org/html/draft-reeder-snmpv3-usm-3desede-00

    Known to be used by many vendors including Cisco and others.
    """
    serviceID = (1, 3, 6, 1, 4, 1, 9, 12, 6, 1, 102)  # cusmAESCfb256PrivProtocol (non-standard OID)
    keySize = 32
