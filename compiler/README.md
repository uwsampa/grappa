# ⚗ Alembic Compiler
One of the trickiest parts of optimizing distributed programs, especially Grappa programs, is determining where data resides and optimizing execution so that tasks typically run close to the data they operate on. In addition, the Grappa programming model requires explicit *delegate* operations whenever accessing remote data via a `GlobalAddress`. The [Alembic][] compiler project automates both of these tasks by introducing the following:

- Global pointer (`global*`) annotation, allowing `GlobalAddress`s to be treated and dereferenced in C++ code just like normal pointers
- Static locality analysis to determine when global memory accesses reside on the same node
- Continuation-passing-style transformation of Grappa tasks to migrate them efficiently to the data they access

This `compiler` branch contains the compiler passes for the [Alembic][] subproject, as well as small changes to the Grappa core to support it. This directory contains the actual LLVM passes that interpret global pointer annotations, analyze locality of tasks, and generate continuation-passing migrations.

This code is research-level only, not production-ready, made available here to allow anyone interested to see the implementation of the LLVM passes. If you wish to learn more, check out the [OOPSLA paper on Alembic][oopsla] or contact [Brandon Holt](mailto:bholt@cs.uw.edu) (@bholt on Github).

[Alembic]: http://homes.cs.washington.edu/~bholt/posts/alembic-appearing-at-oopsla14.html
[oopsla]: http://sampa.cs.washington.edu/papers/oopsla14-alembic.pdf
