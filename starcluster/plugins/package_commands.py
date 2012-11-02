from starcluster import clustersetup
from starcluster.logger import log


class PackageCommands(clustersetup.DefaultClusterSetup):
    """
    This plugin runs apt-get commands on all nodes in the cluster. The
    command is specified in the plugin's config:

    [plugin pkgcommands]
    setup_class = pkgcommands.PackageCommands
    commands = update
    """
    def __init__(self, commands=None):
        super(PackageCommands, self).__init__()
        self.commands = commands
        if commands:
            self.commands = [cmd.strip() for cmd in commands.split(',')]

    def run(self, nodes, master, user, user_shell, volumes):
        if not self.commands:
            log.info("No commands specified!")
            return
        log.info('Running the following apt-get commands on all nodes:')
        log.info(', '.join(self.commands), extra=dict(__raw__=True))
        pkgs = ' '.join(self.commands)
        for node in nodes:
            self.pool.simple_job(node.apt_command, (pkgs), jobid=node.alias)
        self.pool.wait(len(nodes))

    def on_add_node(self, new_node, nodes, master, user, user_shell, volumes):
        log.info('Running the following apt-get commands on %s:' % new_node.alias)
        cmds = ' '.join(self.commands)
        new_node.apt_install(cmds)

    def on_remove_node(self, node, nodes, master, user, user_shell, volumes):
        pass
