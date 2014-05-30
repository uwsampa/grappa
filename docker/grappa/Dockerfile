FROM grappa-cloned

RUN cd grappa/build/Ninja+Release && \
  ninja demo-hello_world

ADD run_hello_world.sh /run_hello_world.sh
RUN chmod +x /run_hello_world.sh

ENTRYPOINT ./run_hello_world.sh
