######################################################
# Docker to build Grappa
######################################################
FROM bholt/grappa-base
MAINTAINER Grappa Team <grappa@cs.washington.edu>

WORKDIR /grappa

CMD ./configure --cc=$(which gcc) --gen=Ninja \
                --third-party=/grappa-third-party \
                --shmmax=$((1<<30)) && \
  cd build/Ninja+Release && \
  ninja
