Notes on how I installed and used Docker and set up the Grappa VM stuff

# OSX

## Quick Start
### Setup docker
First, [install Docker](https://docs.docker.com/installation/mac/), but stop after running the install script -- we will use a custom Vagrant VM rather than boot2docker out of the box. This is because even though Docker containers aren't VM's, Docker only works in Linux, so in OSX, we need a Linux VM to run it. On OSX, Docker uses a VM, and the `docker` command just sends commands to the Docker daemon running in the VM.

On OSX, you'll also want [Vagrant](http://www.vagrantup.com/downloads.html). Quick tip: you can download the .dmg and install, or if you have [homebrew](http://brew.sh/), you can install them using casks:

~~~ bash
> brew tap phinze/homebrew-cask && brew install brew-cask
> brew cask install vagrant
~~~

Now we need to fire up our VM which will run Docker for us.

~~~ bash
# from grappa root
> cd docker

# setup and start our Docker VM
> vagrant up

# verify that the docker command now works
> docker version
Client version: 0.11.1
Client API version: 1.11
Go version (client): go1.2.1
Git commit (client): fb99f99
Server version: 1.1.1
Server API version: 1.13
Git commit (server): bd609d2
Go version (server): go1.2.1
~~~

### Run Grappa in Docker
~~~ bash
# from grappa root:

# download 'grappa-base' image which contains things like gcc, ruby,
# and boost needed to build Grappa
> ./docker/local/build

# run 'configure' from inside the Docker container
> ./docker/local/configure
# verify that it has created a new build directory
> ls build
Ninja+Release

# you can also just launch a shell in the container and run these commands yourself:
> ./docker/local/run

~~~

# Notes

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

	$ export DOCKER_HOST=tcp://localhost:2375
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

> Note: got hit by Docker changing their default port. Or maybe it was boot2docker. Anyway, fixed by `vagrant ssh`ing into boot2docker and calling `docker info` and it listed the sockets, with the port number.

And, docker can also share directories with its own host, which on OSX is the Vagrant boot2docker VM, so to share files all the way from OSX to a Docker container, you must share the shared directory (`/vagrant`) as a volume to the container. For instance, you can run an interactive shell in the `ubuntu` container with a shared volume like so:

	$ docker run -i -t -v /vagrant:/vagrant /bin/bash
	root@ddedd95bfd10:/#
	root@ddedd95bfd10:/# ls /vagrant
	Vagrantfile  hello.txt

And there you have it! Two levels of VM sharing a directory...

Btw, this is what our `Vagrantfile` and `docker/local/run` scripts do to use the container to build from the source directory on the host.

If you get this error (which happens quite frequently for me right now):

    2014/07/10 18:16:06 Error: Cannot start container f1d4be8a78b76a1280ace529a70d760fba418f25d4ac247115e8f0921cdc7907:
    stat /grappa: stale NFS file handle

Then fix it by just restarting the Vagrant VM:

    > vagrant reload


## Running Grappa
- First, you need to re-provision Vagrant's VM to have more memory. Grappa seems to be able to run at SHMMAX=1GB, so I made my VM have 2 GB...
	- (also adjusted cores up to 4... assuming this can be configured in Vagrantfile?)
- run in privileged mode to set SHMMAX for a container:

		$ docker run -ti --privileged grappa-base /bin/bash
		grappa-base$ sysctl -w kernel.shmmax=$((1<<30))

## Additional tools/notes
### Tools To Investigate
- Fig: http://orchardup.github.io/fig/
  - Creates and runs Docker images automatically
- Shipyard: https://github.com/shipyard/shipyard
  - GUI for Docker
- Drone: https://github.com/drone/drone
  - Docker-based Continuous Integration (should work better for us than Jenkins once we have a Dockerfile setup working)


---
[yungsang/boot2docker]: https://vagrantcloud.com/yungsang/boot2docker
