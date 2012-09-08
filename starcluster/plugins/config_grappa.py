"""
StarCluster plugin to perform per-node configuration
for grappa
See: http://sampa.cs.washington.edu/grappa

"""
from starcluster import clustersetup
from starcluster.logger import log
import sys
import subprocess

class SetShmmax(clustersetup.DefaultClusterSetup):
    """
    This plugin sets shmmax to the maximum memory available on
    each node in the cluster unless otherwise specified by shmmaxkB

    [plugin set_shmmax]
    setup_class = softxmt.starcluster.plugins.config_grappa.SetShmmax
optionally:
    shmmaxkB = 123456789

    """
    def __init__(self, shmmaxkB=0):
        super(SetShmmax, self).__init__()
        self.shmmaxkB = shmmaxkB

    def run(self, nodes, master, user, user_shell, volumes):
        shmmax = 1024*self.shmmaxkB
        for node in nodes:
            if shmmax > 0:
                self.pool.simple_job(node.ssh.execute, ('sysctl -w kernel.shmmax=%d' %shmmax), jobid=node.alias)           
            else:
                    self.pool.simple_job(node.ssh.execute, ("sysctl -w kernel.shmmax=`grep MemTotal /proc/meminfo | awk '{print $2*1024}'`"), jobid=node.alias)

        self.pool.wait(len(nodes))

    def on_add_node(self, new_node, nodes, master, user, user_shell, volumes):
        shmmax = 1024*self.shmmaxkB
        if shmmax > 0:
            self.pool.simple_job(new_node.ssh.execute, ('sysctl -w kernel.shmmax=%d' %shmmax), jobid=node.alias)           
        else:
            self.pool.simple_job(new_node.ssh.execute, ("sysctl -w kernel.shmmax=`grep MemTotal /proc/meminfo | awk '{print 1024*$2}'`"), jobid=node.alias)

    def on_remove_node(self, node, nodes, master, user, user_shell, volumes):
        pass
