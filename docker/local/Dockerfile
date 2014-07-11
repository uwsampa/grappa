######################################################
# Docker to build Grappa
######################################################
FROM uwsampa/grappa-base
MAINTAINER Grappa Team <grappa@cs.washington.edu>

WORKDIR /grappa

RUN echo "sudo sysctl -w kernel.shmmax=$((1<<30))" >> /etc/profile

ENTRYPOINT ["/bin/bash"]
CMD ["--login"]
