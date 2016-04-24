Building
===============================================================================

This document describes how to use CMake to perform out-of-source builds. This means that for each configuration, a new "build/*" directory will be created to hold all of the build scripts, temporaries, and built libraries and executables.



Quick start
-------------------------------------------------------------------------------
Example usage:

    ./configure
    cd build/Make+Release
    make

This will build the Grappa static library (in `build/Make+Release/system/libGrappa.a`). To build and run an application or test, use the specific target. It will be built into the same directory hierarchy, but in this build directory. For example:

    # in `build/Make+Release`
    make graph_new.exe
    # generates <project_root>/build/Make+Release/applications/graph500/grappa/graph_new.exe
    # to run, use whatever MPI job launch mechansim is provided on your system. Ours uses Slurm with srun, but there are many variants.
    srun --nodes=4 --ntasks-per-node=4 -- applications/graph500/grappa/graph_new.exe --scale=20 --bench=bfs

This is the simplest build configuration. You are likely to want to specify more things, such as a specific compiler, a directory for already-installed dependencies so you don't have to rebuild them for each new configuration, and more. So read on.


Requirements
-------------------------------------------------------------------------------
You must have a Linux system with the following installed to be able to build Grappa:

* Build system
  * Ruby >= 1.9.3
  * CMake >= 2.8.12
* Compiler
  * GCC >= 4.7.2 (we depend on C++11 features only present in 4.7.2 and newer)
  * Or: Clang >= 3.4
* External:
  * MPI (must support MPI-3)
    * OpenMPI >= 1.7.4
    * MVAPICH2 >= 1.9
    * MPICH >= 3.1
    * Intel MPI >= 5.0.2.044

The following dependencies are dealt with automatically. You may want to override the default behavior for your specific system as described in the next section, especially in the case of Boost (if you already have a copy in a non-standard place).

* Slightly modified versions distributed with Grappa:
  * glog
* Downloaded and built automatically:
  * gflags
  * gperftools (only needed with `GOOGLE_PROFILER ON` or `TRACING` defined)
  * VampirTrace (only needed with `TRACING` defined)
  * Boost (unless you have >= 1.51 already)

To use our test scripts you must have:

* Slurm job manager (in theory just need to be able to launch MPI jobs, but we provide scripts that work with Slurm)

*Note for users on the UW Sampa cluster:* Say ```source /sampa/share/grappa-vars.sh``` before building/using Grappa to pick up pre-build dependences.

*Note for users on the PNNL PIC/PAL clusters:* Say ```module load cmake gcc-4.7.2 mvapich2/1.9b``` before building/using Grappa to pick up pre-build dependences.



Configure
-------------------------------------------------------------------------------
The `configure` script creates a new "build/*" subdirectory and runs CMake to generate build files.

    --gen=[Make]|Ninja|Xcode[,*] Build tool to generate scripts for.
                                   Default: Make.
                                   Can specify multiple with commas.
    --mode=[Release]|Debug[,*]   Build mode. Default: Release (with debug symbols)
    --cc=path/to/c/compiler      Use alternative C compiler.
    --boost=path/to/boost/root   Specify location of compatible boost (>= 1.53)
                                   (otherwise, cmake will download and build it)
    --name=NAME                  Add an additional name to this configuration to distinguish it 
                                   (i.e. compiler version)
    --tracing                    Enable VampirTrace/gperftools-based sampled tracing. Looks for
                                   VampirTrace build in 'third-party' dir.
    --vampir=path/to/vampirtrace/root
                                 Specify path to VampirTrace build (enables tracing).
    --third-party=path/to/built/deps/root
                                 Can optionally pre-build third-party dependencies instead of 
                                   re-building for each configuration.
    --third-party-tarfile=/path/to/file.tar
                                 Instead of downloading third-party dependences from the web, extract them from the specified tar file (available from Grappa website).
    --prefix=path/to/grappa/installation
                                 Specify destination for Grappa installation (defaults to inside build directory).

To build, after calling `configure`, cd into the generated directory, and use the build tool selected (e.g. `make` or `ninja`), specifying the desired target (e.g. `graph_new.exe` to build the new Graph500 implementation, or `check-New_delegate_tests` to run the delegate tests, or `demo-gups.exe` to build the GUPS demo).

### Generators
**Make:** uses generated Unix Makefiles to build. This is currently the best option, as it is the best-tested. Of note, the Make generator seems to be the only one that supports saving the "third-party" builds when running "clean" (which can then be cleaned with the `clean-third-party` target).'

**Ninja:** supposedly does faster dependencies, especially for incremental builds, but `clean` target cleans dependent projects, so not currently recommended.

**Xcode:** seems unable to use anything but Apple's Clang to compile, which is not supported for Grappa yet.

**Others:** untested

### Build Modes
**Release:** builds with `-O3 -g`, so still has debug symbols.

**Debug:** builds with `-DDEBUG -O1`. `-O0` is not currently supported, since inlining is required for our threading library.

## Third-party dependencies
CMake will download and build `gflags`, `boost`, and `gperfools`. It will build and install `GASNet` and `google-glog` from the `third-party/` directory.

The external dependencies can be shared between Grappa configurations. If you specify a directory to `--third-party`, CMake will build and install the dependencies there, and then any other configurations will reuse them. Sometimes this won't work; for instance, if using two different compilers, you may have difficulty sharing a third-party directory. If this happens, just make a new third-party directory and rebuild them using the new configuration, or don't specify it and have this configuration build them just for itself.

Because Boost takes the longest to compile and is often included in systems, Boost can be specified separately from the other third-party installs. Existing system installs of the other dependencies should typically *not* be relied on.

### No web access for third-party dependencies

If you want to build Grappa on a machine without access to the web, and that machine doesn't already have all the third-party libraries installed that Grappa needs, you'll have to provide the source archives for those dependences yourself. 

To do so, download this file: [http://grappa.cs.washington.edu/files/grappa-third-party-downloads.tar](http://grappa.cs.washington.edu/files/grappa-third-party-downloads.tar). Then run ```configure``` with the option ```--third-party-tarfile=</path/to/file.tar>``` pointing at the tarfile.

## Installing Grappa

Grappa supports a ```install``` target which will install compiled libraries and header files to a directory on your system. The default install path is an ```install``` directory under the build directory, like ```build/Make+Release/install```. If you'd prefer it to go somewhere else, specify the ```--prefix=<path>``` option when you run ```configure```.

So to install Grappa to a directory ```/shared/grappa```, do the following in the Grappa source directory:

```
./configure --prefix=/shared/grappa
cd build/Make+Release
make && make install
```

This will result in a tree of directories under ```/shared/grappa``` including libraries, header files, and some scripts.

As part of the install process, Grappa creates a ```bin/settings.sh``` file. After installing Grappa, if you run a command like ```source <grappa install path>/bin/settings.sh```, the environment variable ```GRAPPA_PREFIX``` will be set to the install path, and various other environment variables will be set to help control the runtime and improve performance with certain MPI distributions. 

## External builds / Using Grappa as a library

We've made it easier to use Grappa as a library by providing a GNU Make include file that sets compiler and linker flags appropriately for your Grappa installation. To use it, do the following:

1. Follow the install procedure described in the previous section.
2. Make sure the environment variable ```GRAPPA_PREFIX``` points to your Grappa installation (not build) directory. The simplest way to do this is to source the Grappa settings script:  ```source <grappa install path>/bin/settings.sh```.
3. Include a line like this in your Makefile: ```include $(GRAPPA_PREFIX)/share/Grappa/grappa.mk```
4. Use the Grappa variables in your Makefile rules. There are two ways to do this. You may use GNU Make's implicit rules by setting the variable ```GRAPPA_IMPLICIT_RULES:=on``` before including the Grappa make include file, like this:

   ```
   GRAPPA_IMPLICIT_RULES:=on
   include $(GRAPPA_PREFIX)/share/Grappa/grappa.mk

   standalone: standalone.o
   ```

   or you may use the variables set in the include file directly, like this:

   ```
   include $(GRAPPA_PREFIX)/share/Grappa/grappa.mk

   # Now the following variables are then avaliable to you in writing make rules:
   #      $(GRAPPA_CXX)
   #      $(GRAPPA_CC)
   #      $(GRAPPA_LD)
   #      $(GRAPPA_CXXFLAGS)
   #      $(GRAPPA_LDFLAGS)
   #      $(GRAPPA_LDLIBS)

   standalone: standalone.o
   	$(GRAPPA_LD) $(GRAPPA_LDFLAGS) -o $@ $< $(GRAPPA_LDLIBS)

   standalone.o: standalone.cpp
   	$(GRAPPA_CXX) $(GRAPPA_CXXFLAGS) -o $@ $<
   ```

   An example of the first usage is included in the directory ```applications/demos/standalone```.

## CMake Notes
A couple notes about adding new targets for CMake to build. First: each directory where something is built should typically have a `CMakeLists.txt` file. Somewhere up the directory hierarchy, this directory must be 'added'. For instance, applications directories are added from `applications/CMakeLists.txt`:

    add_subdirectory(graph500)
    add_subdirectory(demos)
    ...

There are a couple custom macros defined for creating Grappa executables:

- `add_grappa_exe(target_name.exe [list of sources])`: creates an executable that links with `libGrappa`, depends on all the "third-party" and has the correct properties set. Our convention is that these use the '.exe' extension, even though we're generally running on Linux.
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
    srun --nodes=2 --ntasks-per-node=1 -- scratch/scratch-test.exe

### Demos
Similar to the scratch directory, all sub-directories in the `applications/demos` directory will be searched for `.cpp` files, and added as targets (`demo-#{base_name}.exe`). (note: search is not recursive, just one level of subdirectory).

### Distributed Build on Sampa
We're no longer using `srun` to distribute build jobs. Instead, it's possible to do a distributed build using `distcc`, it's just a bit more convoluted.

First, you must select the distcc proxy for gcc-4.7.2 that has been set up. If you've already configured, you have to blow the previous one away and start from scratch (see note about changing compiler).

    cd <grappa root directory>
    rm -rf build/Make+Release
    ./configure --gen=Make --mode=Release --cc=/sampa/share/distcc/gcc-4.7.2/bin/gcc

Before building, you must set up a Slurm allocation and launch distcc daemons. Best workflow is to launch 'make' with the slurm allocation, using our helper script:

    # in, for example, build/Make+Release:
    bin/distcc_make -j <target>
    # or for Ninja configurations:
    bin/distcc_ninja <target>

It is possible to control the number of nodes and the Slurm partition to be used by setting environment variables:

| Variable         | Use                                |
|------------------|------------------------------------|
| DISTCC_NNODE     | number of 'distccd' nodes to setup |
| SALLOC_PARTITION | Slurm partition to use             |

Or just invoke salloc directly; e.g.:

    salloc -N8 bin/distcc.sh make -j <target>

Another alternative is to launch a shell to hold onto an allocation. This may be preferable if you are doing frequent builds or if distcc daemons are not loading correctly using the distcc_make option above.

    salloc -N8 <grappa-dir>/bin/distcc.sh bash --login
    cd build/Make+Release
    make -j
    ...
    # whenever finished with builds, relinquish allocation:
    exit

Building documentation
-------------------------------------------------------------------------------
The Grappa system directory is documented with Doxygen comments. To generate the documentation, you must verify that you have Doxygen installed, then build the `docs` target. For example:

```bash
# from build/Make+Release
make docs
```

This will generate doxygen HTML and PDF documentation in: `build/doxygen`. So you can open in your browser: `<grappa_dir>/build/doxygen/html/index.html`.

Testing
-------------------------------------------------------------------------------
See `doc/testing.md`.

Running jobs
-------------------------------------------------------------------------------
See `doc/running.md`.

