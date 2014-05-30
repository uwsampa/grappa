FROM ubuntu

RUN apt-get update
RUN apt-get install -y \
  libboost1.55-all-dev \
  wget \
  make \
  cmake \
  ninja-build \
  git

# build Ruby
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
  ./configure --enable-contrib-no-build=vt && \
  make && \
  make install

# clean up
RUN rm -rf openmpi-1.8.1* ruby-2.1.2*
