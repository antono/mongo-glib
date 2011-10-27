#!/usr/bin/env python

def runTimed(f, *args, **kwargs):
    from datetime import datetime
    start = datetime.now()
    f(*args, **kwargs)
    end = datetime.now()
    return (end - start).total_seconds()

def test1():
    from gi.repository import Mongo

    bson = Mongo.Bson()
    for i in xrange(10000):
        bson.append_int(str(i), i)

def test2():
    import bson as _bson
    seq = ((str(i), i) for i in xrange(10000))
    bson = _bson.BSON.encode(_bson.SON(seq))

def main():
    import sys

    n_runs = 30

    if 'glib' in sys.argv:
        total = 0
        for i in range(n_runs):
            t = runTimed(test1)
            total += t
            print 'Mongo.Bson', t
        print '====='
        print 'Average:', total / float(n_runs)
        sys.exit(0)

    if 'bson' in sys.argv:
        total = 0
        for i in range(n_runs):
            t = runTimed(test2)
            total += t
            print '      bson', t
        print '====='
        print 'Average:', total / float(n_runs)
        sys.exit(0)

    print 'Specify glib or bson'

if __name__ == '__main__':
    main()
