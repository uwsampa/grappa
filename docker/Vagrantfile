# Vagrantfile for running Docker on OSX with NFS.
# 
# Runs the docker daemon in a VM and exports it 
# for the host to connect to (same as `boot2docker`), with the additional feature 
# that this VM (via Vagrant) mounts the current directory via NFS inside the 
# boot2docker VM to allow sharing files between host and containers.

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.define "boot2docker"
  config.vm.box = "yungsang/boot2docker"
  config.vm.network "private_network", ip: "192.168.33.10"
  
  # sync grappa root via nfs
  config.vm.synced_folder "..", "/grappa", type: "nfs"
  
  config.vm.provider "virtualbox" do |v|
    v.memory = 2048
    v.cpus = 4
  end  
  
  # Should be able to redirect docker's port to something else on 
  # the host using the command below. Remember to update `DOCKER_HOST` 
  # environment variable on the host, too.
  #
  # Note: you can determine which port Docker is using with:
  # osxhost> vagrant ssh
  # boot2docker> docker info | grep Sockets
  # Sockets: [unix:///var/run/docker.sock tcp://0.0.0.0:2375]
  #
  # config.vm.network :forwarded_port, guest: 2375, host: 12375
  
end
