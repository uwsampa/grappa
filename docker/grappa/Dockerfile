######################################################
# Base image with all of Grappa's dependencies
######################################################
FROM ubuntu
MAINTAINER Grappa Team <grappa@cs.washington.edu>

RUN apt-get update
RUN apt-get install -y \
  libboost1.55-all-dev \
  wget \
  make \
  cmake \
  ninja-build \
  git \
  openssh-server

# build recent version of Ruby
RUN wget http://cache.ruby-lang.org/pub/ruby/2.1/ruby-2.1.2.tar.gz && \
  tar xzf ruby-2.1.2.tar.gz && \
  cd ruby-2.1.2 && \
  ./configure && \
  make && \
  make install

# build OpenMPI
RUN wget http://www.open-mpi.org/software/ompi/v1.8/downloads/openmpi-1.8.1.tar.gz && \
  tar xzf openmpi-1.8.1.tar.gz && \
  cd openmpi-1.8.1 && \
  CC=$(which gcc) CXX=$(which g++) ./configure --enable-contrib-no-build=vt --prefix=/usr && \
  make && \
  make install

# clean up
RUN rm -rf openmpi-1.8.1* ruby-2.1.2*

# build grappa-third-party libs
RUN git clone https://github.com/uwsampa/grappa.git

RUN grappa/configure --cc=$(which gcc) --gen=Ninja \
              --third-party=/grappa-third-party \
              --shmmax=$((1<<30)) && \
  cd build/Ninja+Release && \
  ninja

ENV LD_LIBRARY_PATH    $LD_LIBRARY_PATH:/usr/local/lib
ENV CC                 /usr/bin/gcc
ENV CXX                /usr/bin/g++
ENV GRAPPA_THIRD_PARTY /grappa-third-party

# have to make sure we re-set shmmax before running any Grappa programs
RUN echo "sudo sysctl -w kernel.shmmax=$((1<<30)) >/dev/null 2>/dev/null" >> /etc/profile

# set prompt colors because I'm a bit vain
RUN echo "export PS1='\[\e[0;34m\]docker \[\e[m\]\[\e[1;32m\]\w\[\e[m\] \[\e[0;33m\]\$ \[\e[m\]'" >> /etc/profile

# containers run from this image will expect to have two volumes available: 
# - /grappa for source code
# - /build for build files (and Makefile, Ninja files, etc)
#
# recommended way to do this is to use the `setup` script to generate data volumes
VOLUME ["/grappa", "/build"]
WORKDIR /build

ENTRYPOINT ["/bin/bash"]
CMD ["--login"]
