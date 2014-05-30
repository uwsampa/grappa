FROM grappa-base

RUN git clone https://github.com/uwsampa/grappa.git

RUN cd grappa && \
  ./configure --cc=$(which gcc) --gen=Ninja \
              --third-party=/grappa-third-party \
              --shmmax=$((1<<30)) && \
  cd build/Ninja+Release && \
  ninja

ENTRYPOINT /bin/bash
