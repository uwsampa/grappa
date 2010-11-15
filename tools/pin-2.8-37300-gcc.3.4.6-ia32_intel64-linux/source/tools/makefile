# This top-level makefile is exported into the Pin kit and is referenced by
# several external scripts.  Be careful when changing the names of the makefile
# targets so as not to break anything that depends on them.
#

# These targets are referenced in the User Guide or are commonly used by
# Pin users.
#
all:    do-build
build:  do-build
test:   do-test-long
	./testsummary
clean:  do-clean


# These targets are used by the "remote-builder.py" script, which is part
# of our sanity testing.
#
sanity-build:       do-build do-build-idb
sanity-test-short:  do-test-short do-test-idb
	./testsummary
sanity-test-long:   do-test-long do-test-idb
	./testsummary
sanity-clean:       do-clean do-clean-idb


# These targets are used by our nightly testing system.
#
nightly-build:  do-build do-build-mypintool do-build-idb
nightly-test:   do-test-long do-test-mypintool do-test-idb
	./testsummary


#
# None of the targets below here are used by anything outside of this makefile.
#

do-build: 
	-$(MAKE) -C Tests test-apps
	-$(MAKE) -C ToolUnitTests
	-$(MAKE) -C SignalTests
	-$(MAKE) -C Debugger
	-$(MAKE) -C Tests
	-$(MAKE) -C SimpleExamples
	-$(MAKE) -C ManualExamples
	-$(MAKE) -C Memory
	-$(MAKE) -C InstLibExamples
	-$(MAKE) -C DebugTrace
	-$(MAKE) -C SegTrace
	-$(MAKE) -C PinPoints
	-$(MAKE) -C CacheClient
	-$(MAKE) -C CodeCacheFootprint
	-$(MAKE) -C Maid
	-$(MAKE) -C Probes
	-$(MAKE) -C AlignChk
	-$(MAKE) -C Mix
	-$(MAKE) -C Insmix
	-$(MAKE) -C SegmentsVirtualization
	-$(MAKE) -C I18N
	-$(MAKE) -C MemTrace
	-$(MAKE) -C ChildProcess
	-$(MAKE) -C CrossIa32Intel64
	-$(MAKE) -C AttachDetach
	-$(MAKE) -C Buffer
	-$(MAKE) -C AVX
	-$(MAKE) -C Replay

do-build-idb:
	-$(MAKE) -C IDBTests

do-build-mypintool:
	-$(MAKE) -C MyPinTool


do-test-short:
	-$(MAKE) -k -C Tests test-apps
	-$(MAKE) -k -C Tests tests-sanity
	-$(MAKE) -k -C ToolUnitTests tests-sanity
	-$(MAKE) -k -C SignalTests tests-sanity
	-$(MAKE) -k -C Debugger tests-sanity
	-$(MAKE) -k -C Probes tests-sanity
	-$(MAKE) -k -C SegTrace tests-sanity
	-$(MAKE) -k -C Mix tests-sanity
	-$(MAKE) -k -C SegmentsVirtualization tests-sanity
	-$(MAKE) -k -C I18N tests-sanity
	-$(MAKE) -k -C ChildProcess tests-sanity
	-$(MAKE) -k -C AttachDetach tests-sanity
	-$(MAKE) -k -C Buffer tests-sanity
	-$(MAKE) -k -C Replay tests-sanity

do-test-long: 
	-$(MAKE) -k -C Tests test-apps
	-$(MAKE) -k -C ToolUnitTests test
	-$(MAKE) -k -C SignalTests test
	-$(MAKE) -k -C Debugger test
	-$(MAKE) -k -C Tests test
	-$(MAKE) -k -C SimpleExamples test
	-$(MAKE) -k -C ManualExamples test
	-$(MAKE) -k -C Memory test
	-$(MAKE) -k -C InstLibExamples test
	-$(MAKE) -k -C DebugTrace test
	-$(MAKE) -k -C SegTrace test
	-$(MAKE) -k -C PinPoints test
	-$(MAKE) -k -C CacheClient test
	-$(MAKE) -k -C CodeCacheFootprint test
	-$(MAKE) -k -C Maid test
	-$(MAKE) -k -C Probes test
	-$(MAKE) -k -C AlignChk test
	-$(MAKE) -k -C Mix  test
	-$(MAKE) -k -C Insmix test
	-$(MAKE) -k -C SegmentsVirtualization test
	-$(MAKE) -k -C I18N test
	-$(MAKE) -k -C MemTrace test
	-$(MAKE) -k -C ChildProcess test
	-$(MAKE) -k -C CrossIa32Intel64 test
	-$(MAKE) -k -C AttachDetach test
	-$(MAKE) -k -C Buffer test
	-$(MAKE) -k -C AVX test
	-$(MAKE) -k -C Replay test

do-test-idb:
	-$(MAKE) -k -C IDBTests test

do-test-mypintool:
	-$(MAKE) -k -C MyPinTool test


do-clean:
	-$(MAKE) -C ToolUnitTests clean
	-$(MAKE) -C SignalTests clean
	-$(MAKE) -C Debugger clean
	-$(MAKE) -C Tests clean
	-$(MAKE) -C SimpleExamples clean
	-$(MAKE) -C ManualExamples clean
	-$(MAKE) -C Memory clean
	-$(MAKE) -C InstLibExamples clean
	-$(MAKE) -C DebugTrace clean
	-$(MAKE) -C SegTrace clean
	-$(MAKE) -C PinPoints clean
	-$(MAKE) -C CacheClient clean
	-$(MAKE) -C CodeCacheFootprint clean
	-$(MAKE) -C Maid clean
	-$(MAKE) -C Probes clean
	-$(MAKE) -C AlignChk clean
	-$(MAKE) -C Mix  clean
	-$(MAKE) -C Insmix clean
	-$(MAKE) -C SegmentsVirtualization clean
	-$(MAKE) -C I18N clean
	-$(MAKE) -C MemTrace clean
	-$(MAKE) -C ChildProcess clean
	-$(MAKE) -C CrossIa32Intel64 clean
	-$(MAKE) -C AttachDetach clean
	-$(MAKE) -C Buffer clean
	-$(MAKE) -C AVX clean
	-$(MAKE) -C Replay clean

do-clean-idb:
	-$(MAKE) -C IDBTests clean
