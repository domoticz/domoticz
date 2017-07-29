#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
# Limited-size dictionary-like class to use for caches
#


class Cache(object):
    def __init__(self, maxSize=256):
        self.__maxSize = maxSize
        self.__size = 0
        self.__chopSize = maxSize // 10
        self.__chopSize = self.__chopSize and self.__chopSize or 1
        self.__cache = {}
        self.__usage = {}

    def __contains__(self, k):
        return k in self.__cache

    def __getitem__(self, k):
        self.__usage[k] += 1
        return self.__cache[k]

    def __len__(self):
        return self.__size

    def __setitem__(self, k, v):
        if self.__size >= self.__maxSize:
            usageKeys = sorted(self.__usage, key=lambda x, d=self.__usage: d[x])
            for _k in usageKeys[:self.__chopSize]:
                del self.__cache[_k]
                del self.__usage[_k]
            self.__size -= self.__chopSize
        if k not in self.__cache:
            self.__size += 1
            self.__usage[k] = 0
        self.__cache[k] = v

    def __delitem__(self, k):
        del self.__cache[k]
        del self.__usage[k]
        self.__size -= 1
