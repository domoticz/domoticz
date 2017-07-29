#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
from bisect import bisect


class OrderedDict(dict):
    """Ordered dictionary used for indices"""

    def __init__(self, **kwargs):
        self.__keys = []
        self.__dirty = True
        super(OrderedDict, self).__init__()
        if kwargs:
            self.update(kwargs)

    def __setitem__(self, key, value):
        if key not in self:
            self.__keys.append(key)
        super(OrderedDict, self).__setitem__(key, value)
        self.__dirty = True

    def __repr__(self):
        if self.__dirty:
            self.__order()
        return super(OrderedDict, self).__repr__()

    def __str__(self):
        if self.__dirty:
            self.__order()
        return super(OrderedDict, self).__str__()

    def __delitem__(self, key):
        if super(OrderedDict, self).__contains__(key):
            self.__keys.remove(key)
        super(OrderedDict, self).__delitem__(key)
        self.__dirty = True

    __delattr__ = __delitem__

    def clear(self):
        super(OrderedDict, self).clear()
        self.__keys = []
        self.__dirty = True

    def keys(self):
        if self.__dirty:
            self.__order()
        return list(self.__keys)

    def values(self):
        if self.__dirty:
            self.__order()
        return [self[k] for k in self.__keys]

    def items(self):
        if self.__dirty:
            self.__order()
        return [(k, self[k]) for k in self.__keys]

    def update(self, d):
        [self.__setitem__(k, v) for k, v in d.items()]

    def sortingFun(self, keys):
        keys.sort()

    def __order(self):
        self.sortingFun(self.__keys)
        d = {}
        for k in self.__keys:
            d[len(k)] = 1
        l = list(d.keys())
        l.sort(reverse=True)
        self.__keysLens = tuple(l)
        self.__dirty = False

    def nextKey(self, key):
        keys = list(self.keys())
        if key in self:
            nextIdx = keys.index(key) + 1
        else:
            nextIdx = bisect(keys, key)
        if nextIdx < len(keys):
            return keys[nextIdx]
        else:
            raise KeyError(key)

    def getKeysLens(self):
        if self.__dirty:
            self.__order()
        return self.__keysLens


class OidOrderedDict(OrderedDict):
    """OID-ordered dictionary used for indices"""

    def __init__(self, **kwargs):
        self.__keysCache = {}
        OrderedDict.__init__(self, **kwargs)

    def __setitem__(self, key, value):
        if key not in self.__keysCache:
            if isinstance(key, tuple):
                self.__keysCache[key] = key
            else:
                self.__keysCache[key] = [int(x) for x in key.split('.') if x]
        OrderedDict.__setitem__(self, key, value)

    def __delitem__(self, key):
        if key in self.__keysCache:
            del self.__keysCache[key]
        OrderedDict.__delitem__(self, key)

    __delattr__ = __delitem__

    def sortingFun(self, keys):
        keys.sort(key=lambda k, d=self.__keysCache: d[k])
