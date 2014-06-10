# Vagrantfile for running Docker on OSX with NFS.
# 
# Runs the docker daemon in a VM and exports it 
# for the host to connect to (same as `boot2docker`), with the additional feature 
# that this VM (via Vagrant) mounts the current directory via NFS inside the 
# boot2docker VM to allow sharing files between host and containers.

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  
  # Build VM from an NFS-enabled boot2docker.
  config.vm.box = "yungsang/boot2docker"
  
  config.vm.synced_folder ".", "/vagrant", type: "nfs"

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  config.vm.network "private_network", ip: "192.168.33.10"

  # Should be able to redirect docker's port (4234?) to something else on 
  # the host using the command below. Remember to update `DOCKER_HOST` 
  # environment variable on the host, too.
  # config.vm.network :forwarded_port, guest: 4234, host: 14234
  
end
