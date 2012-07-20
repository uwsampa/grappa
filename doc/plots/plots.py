#!/usr/bin/env python2.7

import sys
import getopt
import exceptions

import sqlite3

import matplotlib.pyplot as plt
#from matplotlib.figure import Figure                       
#from matplotlib.axes import Axes                           
#from matplotlib.lines import Line2D                        
#from matplotlib.backends.backend_agg import FigureCanvasAgg


# databases
bholt_db = "./grappa-cmb.db"
bdmyers_db = "./uts.db"



def get_tables(cursor):
    sql = "select tbl_name from sqlite_master where type = 'table' and tbl_name not like 'sqlite%'"
    cursor.execute(sql)
    return [ f[0] for f in cursor.fetchall() ]

def get_fields(cursor, table):
    sql = "pragma table_info(%s)" % table
    cursor.execute(sql)
    return [ f[1] for f in cursor.fetchall() ]

def get_all(cursor):
    for t in get_tables(c):
        print "For table %s:" % t
        for f in get_fields(c, t):
            print " field %s" % f

def stringToNum(f,s):
    if s is None:
        return 0
    else:
        try:
            return f(s)
        except exceptions.ValueError:
            return 0

def get_values(cursor, table, metric, order):
    sql = "select %s from %s order by %s" % (metric, table, order)
    cursor.execute(sql)
    return [ f[0] for f in cursor.fetchall() ]

def indices(xs):
    return [x[0] for x in xs]
def values(xs):
    return [x[1] for x in xs]

def graph500(c):
    def get(machine, scale):
        c.execute( """
                   SELECT nnode, MAX( max_teps )
                   FROM graph500 
                   WHERE machine = ?
                   AND code is null
                   AND max_teps is not null
                   AND scale = ?
                   GROUP BY nnode
                   ORDER BY nnode
                   """, (machine,scale) )
        return c.fetchall()

    pal = get('pal', 27)
    cougarxmt = get('cougarxmt', 27)

    if True:
        plt.plot( indices(pal), values(pal), label="Grappa" )
        plt.plot( indices(cougarxmt), values(cougarxmt), label="XMT" )
        plt.title('BFS performance')
        plt.xlabel('Number of network interfaces')
        plt.ylabel('TEPS')
        plt.legend( ["Grappa", "XMT"], loc=0 )
        plt.show()

    
def suite(c):
    def get(machine, scale):
        c.execute( """
                   SELECT nnode, (kcent * (1 << scale) * 16) / MIN( centrality_time ) 
                   FROM suite
                   WHERE machine = ?
                   AND centrality_time is not null
                   AND scale = ?
                   GROUP BY nnode
                   ORDER BY nnode
                   """, (machine,scale) )
        return c.fetchall()

    pal = get('pal', 27)
    cougarxmt = get('cougarxmt', 27)

    if True:
        plt.plot( indices(pal), values(pal), label="Grappa" )
        plt.plot( indices(cougarxmt), values(cougarxmt), label="XMT" )
        plt.title('Betweenness Centrality performance')
        plt.xlabel('Number of network interfaces')
        plt.ylabel('TEPS')
        plt.legend( ["Grappa", "XMT"], loc=0 )
        plt.show()



def uts(c):
    def get_xmt(tree):
        c.execute( """
                   SELECT num_places, nNodes / MIN( runtime )
                   FROM uts_xmt
                   WHERE tree = ?
                   AND num_places is not 'true'
                   GROUP BY num_places
                   ORDER BY num_places
                   """, (tree,) )
        return c.fetchall()

    def get_grappa(ppn, tree):
        c.execute( """
                   SELECT num_places, nNodes / MIN( search_runtime )
                   FROM uts_pal
                   WHERE tree = ?
                   AND procs_per_node = ?
                   AND num_places is not 'true'
                   GROUP BY num_places
                   ORDER BY num_places
                   """, (tree, ppn,) )
        return c.fetchall()

    def plot_grappa( ppn, tree ):
        g = get_grappa( ppn, tree )
        plt.plot( indices(g), values(g), label="Grappa, %d PPN, %s" % (ppn, tree) )

    def plot_xmt( tree ):
        g = get_xmt( tree )
        plt.plot( indices(g), values(g), label="XMT, %s" % tree )

    if True:
        plot_grappa( 4, 'T1L' )
        plot_grappa( 5, 'T1L' )
        plot_grappa( 6, 'T1L' )
        plot_grappa( 4, 'T1XL' )
        plot_grappa( 5, 'T1XL' )
        plot_grappa( 6, 'T1XL' )
        plot_grappa( 4, 'T1XXL' )
        plot_grappa( 5, 'T1XXL' )
        plot_xmt( 'T1L' )
        plot_xmt( 'T1XL' )
        plot_xmt( 'T1XXL' )

        plt.title('Unbalanced Tree Search performance')
        plt.xlabel('Number of network interfaces')
        plt.ylabel('TEPS')
        #plt.legend( loc=0 )
        plt.show()



def dowork():
    bholt_conn = sqlite3.connect( bholt_db )
    bholt_c = bholt_conn.cursor()
    #graph500(bholt_c)
    #suite(bholt_c)
    bholt_c.close()

    bdmyers_conn = sqlite3.connect( bdmyers_db )
    bdmyers_c = bdmyers_conn.cursor()
    uts(bdmyers_c)
    bdmyers_c.close()







class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

def main(argv=None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "h", ["help"])
        except getopt.error, msg:
             raise Usage(msg)
        dowork()
    except Usage, err:
        print >>sys.stderr, err.msg
        print >>sys.stderr, "for help use --help"
        return 2

if __name__ == "__main__":
    sys.exit(main())

