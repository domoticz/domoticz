#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from pysnmp.proto import error
from pysnmp import nextid


class Cache(object):
    __stateReference = nextid.Integer(0xffffff)
    __msgID = nextid.Integer(0xffffff)

    def __init__(self):
        self.__msgIdIndex = {}
        self.__stateReferenceIndex = {}
        self.__sendPduHandleIdx = {}
        # Message expiration mechanics
        self.__expirationQueue = {}
        self.__expirationTimer = 0

    # Server mode cache handling

    def newStateReference(self):
        return self.__stateReference()

    def pushByStateRef(self, stateReference, **msgInfo):
        if stateReference in self.__stateReferenceIndex:
            raise error.ProtocolError('Cache dup for stateReference=%s at %s' % (stateReference, self))
        expireAt = self.__expirationTimer + 600
        self.__stateReferenceIndex[stateReference] = msgInfo, expireAt

        # Schedule to expire
        if expireAt not in self.__expirationQueue:
            self.__expirationQueue[expireAt] = {}
        if 'stateReference' not in self.__expirationQueue[expireAt]:
            self.__expirationQueue[expireAt]['stateReference'] = {}
        self.__expirationQueue[expireAt]['stateReference'][stateReference] = 1

    def popByStateRef(self, stateReference):
        if stateReference in self.__stateReferenceIndex:
            cacheInfo = self.__stateReferenceIndex[stateReference]
        else:
            raise error.ProtocolError('Cache miss for stateReference=%s at %s' % (stateReference, self))
        del self.__stateReferenceIndex[stateReference]
        cacheEntry, expireAt = cacheInfo
        del self.__expirationQueue[expireAt]['stateReference'][stateReference]
        return cacheEntry

    # Client mode cache handling

    def newMsgID(self):
        return self.__msgID()

    def pushByMsgId(self, msgId, **msgInfo):
        if msgId in self.__msgIdIndex:
            raise error.ProtocolError(
                'Cache dup for msgId=%s at %s' % (msgId, self)
            )
        expireAt = self.__expirationTimer + 600
        self.__msgIdIndex[msgId] = msgInfo, expireAt

        self.__sendPduHandleIdx[msgInfo['sendPduHandle']] = msgId

        # Schedule to expire
        if expireAt not in self.__expirationQueue:
            self.__expirationQueue[expireAt] = {}
        if 'msgId' not in self.__expirationQueue[expireAt]:
            self.__expirationQueue[expireAt]['msgId'] = {}
        self.__expirationQueue[expireAt]['msgId'][msgId] = 1

    def popByMsgId(self, msgId):
        if msgId in self.__msgIdIndex:
            cacheInfo = self.__msgIdIndex[msgId]
        else:
            raise error.ProtocolError(
                'Cache miss for msgId=%s at %s' % (msgId, self)
            )
        msgInfo, expireAt = cacheInfo
        del self.__sendPduHandleIdx[msgInfo['sendPduHandle']]
        del self.__msgIdIndex[msgId]
        cacheEntry, expireAt = cacheInfo
        del self.__expirationQueue[expireAt]['msgId'][msgId]
        return cacheEntry

    def popBySendPduHandle(self, sendPduHandle):
        if sendPduHandle in self.__sendPduHandleIdx:
            self.popByMsgId(self.__sendPduHandleIdx[sendPduHandle])

    def expireCaches(self):
        # Uses internal clock to expire pending messages
        if self.__expirationTimer in self.__expirationQueue:
            cacheInfo = self.__expirationQueue[self.__expirationTimer]
            if 'stateReference' in cacheInfo:
                for stateReference in cacheInfo['stateReference']:
                    del self.__stateReferenceIndex[stateReference]
            if 'msgId' in cacheInfo:
                for msgId in cacheInfo['msgId']:
                    del self.__msgIdIndex[msgId]
            del self.__expirationQueue[self.__expirationTimer]
        self.__expirationTimer += 1
