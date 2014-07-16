######################################################
# Docker to build Grappa
######################################################
FROM uwsampa/grappa-base
MAINTAINER Grappa Team <grappa@cs.washington.edu>

VOLUME ["/grappa", "/build"]
WORKDIR /build

# have to make sure we re-set shmmax before running any Grappa programs
RUN echo "sudo sysctl -w kernel.shmmax=$((1<<30))" >> /etc/profile

# set prompt colors because I'm a bit vain
RUN echo "export PS1='\[\e[0;34m\]docker \[\e[m\]\[\e[1;32m\]\w\[\e[m\] \[\e[0;33m\]\$ \[\e[m\]'" >> /etc/profile

ENTRYPOINT ["/bin/bash","--login"]
