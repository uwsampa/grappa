Testing in Grappa
===============================================================================
We use Boost::Test to test our code.

The full list of unit tests is found in `system/CMakeLists.txt`. Here, a macro `add_check` is used to define a test and tell whether it is currently expected to pass or fail.

Each test defined in this way creates two targets: `*.test` which builds the test, and `check-*`, which runs the test. In addition, there are aggregate targets `check-all-{pass,fail}` which build and run all the passing or failing tests respectively, and `check-all-{pass,fail}-compile-only` which, as the name implies, only compiles them.

Non-exhaustive list of test targets:
- `New_loop_tests.test`: build loop tests
- `check-New_loop_tests`: build and run loop tests
- `check-all-pass`: build and run all passing tests
- `check-all-pass-compile-only`: just build all the tests expected to pass

Someday we'll get this up and running with some CI server, but until then, we just try and run it whenever we make significant changes.
