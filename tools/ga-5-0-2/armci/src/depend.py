#!/usr/bin/python

'''
Simple dependency tracking. Lists, in make understandable format, all
local header files included in source files. The object file names are
given as arguments.

@author Sriram Krishnamoorthy
'''

import dircache
import re
import sys

'''All files in current directory'''
def headersInDir(directory):
    return dircache.listdir(directory)

'''All header files included in a given file'''
def includesInFile(fileName):
    rval=set();
    f = open(fileName);
    try:
        for line in f:
            ex = re.compile('[\s]*#[\s]*include[\s]*(<|")([\S]*)(>|")[\s]*'); 
            m = ex.match(line);
            if(m != None):
                #print m.group(2);
                rval.add(m.group(2));
    finally:
        f.close();
    return rval;

'''All header files included in given file, that are in the same directory'''
def localIncludes(localHeaders,fileName):
    rval = set();
    includes = includesInFile(fileName);
    for f in includes:
        if ( f in localHeaders):
            rval.add(f);
    return rval;

'''Print in format usable by make'''
def prettyPrint(fileName, localHeaders):
    name = fileName.rsplit('.',1);
    obj = '.'.join(name[0:len(name)-1] + ['o']);
    sys.stdout.write('$(FULL_LIBRARY_PATH)(%s):  ' % obj);
    for h in localHeaders:
        sys.stdout.write('./%s ' % h);
    sys.stdout.write('\n');

'''Find source file in directory listing given an object file'''
def findSourceFile(dirlist, objectFile):
    ex = re.compile('([\S]*)\.o');
    m = ex.match(objectFile);
    if m is None or m.group(1) is None:
        return None;
    baseName = m.group(1);
    ex = re.compile(baseName+'\.(c|f|F|gcc)');
    for fl in dirlist:
        if (ex.match(fl)):
            return fl;
    return None;

'''Transitive closure of a collection of sets. Argument is a
dictionary of (name,set) items'''
def transitiveClosure(hdrs):
    for incl in hdrs.keys():
        hdrs[incl].add(incl);
    for incl1 in hdrs.keys():
        for incl2 in hdrs.keys():
            for incl3 in hdrs.keys():
                if incl3 not in hdrs[incl2]:
                    if(incl1 in hdrs[incl2] and incl3 in hdrs[incl1]):
                        hdrs[incl2].add(incl3);
    for incl in hdrs.keys():
        hdrs[incl].remove(incl);

'''Compute and print local dependences'''
def mkdep(directory, srcs):
    imm_deps = {};
    hdrs = {};
    dirlist = headersInDir(directory);
#   Immediate dependences (files included in source)
    for src in srcs:
        imm_deps[src] = localIncludes(dirlist,src);
#   Collection of all included files
    for (src,includes) in imm_deps.iteritems():
        for incl in includes:
            if incl not in hdrs:
                hdrs[incl] = localIncludes(dirlist, incl);
    transitiveClosure(hdrs);
#   Collection of all header files (transitively) included from src
    deps = {};
    for (src,includes) in imm_deps.iteritems():
        deps[src] = set();
        deps[src].update(includes);
        for incl in includes:
            deps[src].update(hdrs[incl]);
    for (src,includes) in deps.iteritems():
        prettyPrint(src, includes);

if __name__ == "__main__":
    mkdep('.',sys.argv[1:]);
        
