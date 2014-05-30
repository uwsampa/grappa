Notes on how I installed and used Docker and set up the Grappa VM stuff

# OSX
Docker doesn't run natively on OSX, but they have nice support via a lightweight `boot2docker` VM to run Docker in a VM and proxy those through the `docker` command in OSX.

Sharing files between them, however, is not supported by default. I got this working using Vagrant's NFS support. Vagrant recommends NFS over VirtualBox shared folders for better performance, and they make it really simple to set up. To set up the NFS-enabled boot2docker VM, I started from [yungsang/boot2docker][]'s VM.

	$ mkdir /opt/docker
	$ vagrant init yungsang/boot2docker

This downloads a `Vagrantfile` to your current directory, which I then needed to adjust to enable NFS by uncommenting/adding these two lines:

~~~ ruby
# share current directory (/opt/docker in this example) with the VM, mounted at /vagrant
config.vm.synced_folder ".", "/vagrant", type: "nfs"
# Vagrant needs a private host-only network interface for running the NFS shared folder
config.vm.network "private_network", ip: "192.168.33.10"
~~~

And then fire up the VM (downloads and configures and launches the VM defined by the Vagrantfile):

	$ vagrant up

Test the shared directory:

	$ echo hello > hello.txt
	$ vagrant ssh
	docker@boot2docker:~$ ls /vagrant
	Vagrantfile  hello.txt

To run Docker from OSX, then, tell the docker proxy how to connect to the boot2docker VM, and try it out:

	$ export DOCKER_HOST=tcp://localhost:4243
	$ docker version
	Client version: 0.11.1
	Client API version: 1.11
	Go version (client): go1.2.1
	Git commit (client): fb99f99
	Server version: 0.11.1
	Server API version: 1.11
	Git commit (server): fb99f99
	Go version (server): go1.2.1
	Last stable version: 0.11.1

And, docker can also share directories with its own host, which on OSX is the Vagrant boot2docker VM, so to share files all the way from OSX to a Docker container, you must share the shared directory (`/vagrant`) as a volume to the container. For instance, you can run an interactive shell in the `ubuntu` container with a shared volume like so:

	$ docker run -i -t -v /vagrant:/vagrant /bin/bash
	root@ddedd95bfd10:/#
	root@ddedd95bfd10:/# ls /vagrant
	Vagrantfile  hello.txt

And there you have it! Two levels of VM sharing a directory...

# Running Grappa
- First, you need to re-provision Vagrant's VM to have more memory. Grappa seems to be able to run at SHMMAX=1GB, so I made my VM have 2 GB...
	- (also adjusted cores up to 4... assuming this can be configured in Vagrantfile?)
- run in privileged mode to set SHMMAX for a container:

		$ docker run -ti --privileged grappa-base /bin/bash
		grappa-base$ sysctl -w kernel.shmmax=$((1<<30))



---
[yungsang/boot2docker]: https://vagrantcloud.com/yungsang/boot2docker
