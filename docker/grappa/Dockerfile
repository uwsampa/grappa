######################################################
# Docker to build Grappa
######################################################
FROM bholt/grappa-base
MAINTAINER Grappa Team <grappa@cs.washington.edu>

RUN git clone https://github.com/uwsampa/grappa.git

RUN cd grappa && \
  ./configure --cc=$(which gcc) --gen=Ninja \
              --third-party=/grappa-third-party \
              --shmmax=$((1<<30)) && \
  cd build/Ninja+Release && \
  ninja

RUN cd grappa/build/Ninja+Release && \
  ninja demo-hello_world

ADD run_hello_world.sh /run_hello_world.sh
RUN chmod +x /run_hello_world.sh

# have to make sure we re-set shmmax before running any Grappa programs
RUN echo "sudo sysctl -w kernel.shmmax=$((1<<30))" >> /etc/profile

ENTRYPOINT /bin/bash --login
