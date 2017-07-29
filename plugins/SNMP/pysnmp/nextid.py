#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#
import random

random.seed()


class Integer(object):
    """Return a next value in a reasonably MT-safe manner"""

    def __init__(self, maximum, increment=256):
        self.__maximum = maximum
        if increment >= maximum:
            increment = maximum
        self.__increment = increment
        self.__threshold = increment // 2
        e = random.randrange(self.__maximum - self.__increment)
        self.__bank = list(range(e, e + self.__increment))

    def __repr__(self):
        return '%s(%d, %d)' % (
            self.__class__.__name__,
            self.__maximum,
            self.__increment
        )

    def __call__(self):
        v = self.__bank.pop(0)
        if v % self.__threshold:
            return v
        else:
            # this is MT-safe unless too many (~ increment/2) threads
            # bump into this code simultaneously
            e = self.__bank[-1] + 1
            if e > self.__maximum:
                e = 0
            self.__bank.extend(range(e, e + self.__threshold))
            return v
