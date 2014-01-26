Debugging
===============================================================================

First of all, Grappa is a very young system, so there are likely to be many bugs, and some functionality is particularly brittle at the moment. In the course of debugging our own programs, we have found ways to debug:

* Build with `./configure --mode=Debug` to get better stack traces (note, you'll have to use a different CMake-generated build directory, but this prevents confusing situations where not all files were built for debug).
* The Google logging library we use is *really* good at getting things in order and flushing correctly. Use them and trust them. Debugging verbosity can be changed per-file with `--vmodule`. See [their documentation](http://google-glog.googlecode.com/svn/trunk/doc/glog.html).
* GASNet has support for suspending applications to allow you to attach to them in gdb. Calling `grappa_srun` with `--freeze-on-error` will enable this feature.
* **(TO BE FIXED SOON)** `system/grappa_gdb.macros`: Some useful macros for introspection into grappa data structures. Also allows you to switch to a running task and see its stack. Add the macro to your `.gdbinit` and type `help grappa` in gdb to see commands and usage.

Performance debugging tips
-------------------------------------------------------------------------------

* Grappa has a bunch of statistics that can be dumped (`Grappa::Metrics::merge_and_print()`), use these to find out basic coarse-grained information. You can also easily add your own using `GRAPPA_DEFINE_METRIC()`.
* Grappa also supports collecting traces of the statistics over time using VampirTrace. These can be visualized in Vampir. Build with `VAMPIR_TRACE=1` and `GOOGLE_SAMPLED=1`.

Tracing
-------------------------------------------------------------------------------

Grappa supports sampled tracing via VampirTrace. These traces can be visualized using the commercial tool, [Vampir](http://www.vampir.eu/). When tracing, what happens is all of the metrics specified with `GRAPPA_DEFINE_METRIC()` are sampled at a regular interval using sampling interrupts by Google's `gperftools` library, and saved to a compressed trace using the VampirTrace open source library. This results in a trace with the individual values of all of the Grappa statistics on all cores, over the execution.

Before configuring, ensure you have VampirTrace built somewhere. If VampirTrace is not built anywhere, there is a script to download and build it in `tools/`. Make sure to specify where to install it with the `--prefix` flag:

```bash
> ./tools/vampirtrace.rb --prefix=/sampa/share/grappa-third-party
```

To use tracing, a separate Grappa build configuration must be made using either the '--tracing' or '--vampir' flags. If Vampir was installed in your `third-party` directory, just use '--tracing'; otherwise, use '--vampir' to specify the path to a separate VampirTrace install. For example:

```bash
> ./configure --mode=Release --vampir=/opt/vampirtrace --third-party=/opt/grappa-third-party
```

Tracing configurations have `+Tracing` in their name. Cd into the build directory and build and run as usual. After running a Grappa executable, you should see trace output files show up in your build directory:

* `${exe}.otf`: main file that links other trace files together
* `${exe}.*.events.z`: zipped trace events per process

For now, tracing must be explicitly enabled for a particular region of execution, using `Metrics::start_tracing()` and `Metrics::stop_tracing()`. For example:

```cpp
int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    // setup, allocate global memory, etc
    
    Metrics::start_tracing();
    
    // performance-critical region
    
    Metrics::stop_tracing();
  });
  finalize();
}
```

