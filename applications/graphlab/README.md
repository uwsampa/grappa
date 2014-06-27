GraphLab API in Grappa
----------------------

This directory contains code to emulate the [GraphLab][] API with a simple layer on top of Grappa. This API is not perfectly compatible with GraphLab code, but the example vertex programs in this directory are mostly faithful to those in GraphLab proper.

There are currently two implementations:

- `NaiveGraphlabEngine` (`graphlab_naive.hpp`): implements a restricted GraphLab API using the builtin Grappa Graph structure. Most notably, only `gather:IN_EDGES` and `scatter:OUT_EDGES` are supported.

- `GraphlabEngine` (`graphlab_splitv.hpp`): built on a custom graph structure mimicking GraphLab's greedy vertex-split representation. This is currently slower, and still does not implement the full range of options. `pagerank_new.cpp` is an example that uses this engine.

[GraphLab]: graphlab.org