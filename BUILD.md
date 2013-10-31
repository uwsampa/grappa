# Building
Note: currently Grappa does not support any 'install' step. To use, build using the instructions below. New applications can be added to the applications folder, using the existing applications' `CMakeLists.txt` files as an example, and built using Grappa's CMake project.

This document describes how to use CMake to perform out-of-source builds. This means that for each configuration, a new "build/*" directory will be created to hold all of the build scripts, temporaries, and built libraries and executables.

## Quick start
Example usage:

    ./configure --gen=Make --mode=Release
    cd build/Make+Release
    make -j

This will build the Grappa static library (in `build/Make+Release/system/libGrappa.a`). To build and run an application or test, use the specific target. It will be built into the same directory hierarchy, but in this build directory. For example:

    # in `build/Make+Release`
    make graph_new.exe
    # generates <project_root>/build/Make+Release/applications/graph500/grappa/graph_new.exe
    # to run, use the 'srun' script which has been copied to build/Make+Release/bin:
    bin/grappa_srun.rb --nnode=4 --ppn=4 -- applications/graph500/grappa/graph_new.exe --scale=20 --bench=bfs

## Requirements
- CMake version >= 2.8.6
  - on the Sampa cluster: `/sampa/share/cmake/bin/cmake`
  - on Pal: `module load cmake`
- GCC version >= 4.7.
  - on the Sampa cluster: `/sampa/share/gcc-4.7.2/rtf/bin/{gcc,g++}`
  - on Pal: `module load gcc-4.7.2`
- MPI (tested with OpenMPI & Mvapich2)
  - on Sampa: should be autodetected
  - on Pal: `module load mvapich2/1.9b`

## Configure

The `configure` script is used to create "build/*" subdirectories and run CMake to generate build files.

    Usage: ./configure [options]
    Options summary: (see more details in INSTALL.md)
        --gen=[Make]|Ninja|Xcode[,*] Build tool to generate scripts for.
                                       Default: Make.
                                       Can specify multiple with commas.
        --mode=[Release]|Debug[,*]   Build mode. Default: Release (with debug symbols)
        --cc=path/to/c/compiler      Use alternative C compiler.
        --cxx=path/to/c++/compiler   Alternative C++ compiler (must match --cc choice)

To build, after calling `configure`, change into the generated directory, and use the build tool selected (e.g. `make` for make), specifying the desired target (e.g. `graph_new.exe` to build the new Graph500 implementation, or `check-New_delegate_tests` to run the delegate tests, or `demo-gups.exe` to build the GUPS demo).

### Generators
**Make:** uses generated Unix Makefiles to build. This is currently the best option, as it is the best-tested. Of note, the Make generator seems to be the only one that supports saving the "tools" builds when running "clean" (which can then be cleaned with the `clean-tools` target).'

**Ninja:** supposedly does faster dependencies, especially for incremental builds, but `clean` target cleans dependent projects, so not currently recommended.

**Xcode:** seems unable to use anything but Apple's Clang to compile, which is not supported for Grappa yet.

**Others:** untested

### Build Modes
**Release:** builds with `-O3 -g`, so still has debug symbols.

**Debug:** builds with `-DDEBUG -O1`

## Tools: (external dependencies)
CMake will download and build `gflags`, `boost`, and `gperfools`. It will build and install `GASNet` and `google-glog` from the `tools/` directory.

*Currently, these will have to be re-built for each project/configuration.* CMake needs to know it has an up-to-date version built, and I haven't been able to figure out how to get them to share. However, it is only building the "seq" targets of GASNet and only building the Boost libraries we use, so hopefully it won't take too long to build each time (or take up too much space).

## CMake Notes
A couple notes about adding new targets for CMake to build. First: each directory where something is built should typically have a `CMakeLists.txt` file. Somewhere up the directory hierarchy, this directory must be 'added'. For instance, applications directories are added from `applications/CMakeLists.txt`:

    add_subdirectory(graph500)
    add_subdirectory(demos)
    ...

There are a couple custom macros defined for creating Grappa executables:

- `add_grappa_exe(target_name.exe [list of sources])`: creates an executable that links with `libGrappa`, depends on all the "tools" and has the correct properties set. Convention is that these use the '.exe' extension.
- `add_grappa_application(target_name [list of sources])`: same as `add_grappa_exe` but sets a property so it is tagged as an 'Application' in Xcode/VS.
- `add_grappa_test(target_name.test [list of sources])`: builds/links as a Boost Unit Test. Convention is to use '.test' for the extension. This will also create a second target: `check-#{target_name}` (without '.test'), which will build and run the test. This target also adds itself to the list of all test, which can be run with the `check-all` target.

**Note:** the generated build scripts (e.g. "make") will re-run `cmake` if any CMakeLists.txt files have changed. It can also be invoked directly to, for example, create a target to run, by calling:

    cmake ../..
    # or the `rebuild_cache` target, for example:
    make rebuild_cache

Note: the path must point to the directory where the root CMakeLists file is (in this case, from build/Make+Release, 2 directories up).

Another option sometimes useful is to use the "curses"-based UI, invoked by:

    ccmake ../..

### Starting fresh / changing compiler
If you want to re-configure some configuration from scratch, the best thing to do is delete the entire directory and re-run `./configure`. **You must do this if you want to change anything about the compiler.**

### Verbose
To get more output:
- When running CMake directly: `cmake --debug`
- When using `--gen=Make`: `make VERBOSE=1`
- When using `--gen=Ninja`: `ninja -v`

## Miscellaneous features
### Scratch directory
To make it easy to prototype ideas, there's a directory in root: `scratch`. Any `#{base_name}.cpp` files put here will automatically get their own target: `scratch-#{base_name}.exe`.

**Note:** in order to pick up the addition of a new file, `cmake` must be invoked, otherwise it will have no way to detect the new dependence. To do this from a build directory such as `build/Make+Release`:

    make rebuild_cache
    # then...
    make scratch-test.exe
    bin/grappa_srun.rb --nnode=2 --ppn=1 -- scratch/scratch-test.exe

### Demos
Similar to the scratch directory, all sub-directories in the `applications/demos` directory will be searched for `.cpp` files, and added as targets (`demo-#{base_name}.exe`). (note: search is not recursive, just one level of subdirectory).

### Distributed Build on Sampa
We're no longer using `srun` to distribute build jobs. Instead, it's possible to do a distributed build using `distcc`, it's just a bit more convoluted.

First, you must select the distcc proxy for gcc-4.7.2 that has been set up. If you've already configured, you have to blow the previous one away and start from scratch (see note about changing compiler).

    cd <grappa root directory>
    rm -rf build/Make+Release
    ./configure --gen=Make --mode=Release --cc=/sampa/share/distcc/gcc-4.7.2/bin/gcc --cxx=/sampa/share/distcc/gcc-4.7.2/bin/g++
    cd build/Make+Release

Before building, you must set up a Slurm allocation and launch distcc daemons. Best workflow is to launch 'make' with the slurm allocation, using our helper script:

    # in, for example, build/Make+Release:
    bin/distcc_make -j <target>
    
    # or to control the allocation (such as number of nodes),
    # invoke the salloc job with make manually:
    salloc -N8 bin/distcc.sh make -j

Another alternative is to launch a shell to hold onto an allocation. This may be preferable if you are doing frequent builds:

    salloc -N8 <grappa-dir>/bin/distcc.sh bash --login
    cd build/Make+Release
    make -j
    ...
    # whenever finished with builds, relinquish allocation:
    exit

Or use the helper script:

    # equivalent to 'salloc -N8 bin/distcc.sh bash --login'
    bin/distcc_bash

This will relinquish the Slurm job immediately after the command returns.

**TODO:** come up with way to get distcc allocation without using a second shell (use sbatch to get a persistent job...)

    sbatch --input=/dev/random bin/distcc.sh
    # get nodelist somehow...

