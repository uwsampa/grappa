
# nelson@n71:~/grappa/system$ make -f ../tools/vtune.mk TARGET=scratch/foo.exe NNODE=1 vtune_mpi_test

include ../system/Makefile

ENV_VARIABLES+= LM_LICENSE_FILE="\$$LM_LICENSE_FILE:28518@10.1.2.10:28518@10.1.2.2:28518@n10:28518@n02"
CFLAGS+= -I/sampa/share/intel/vtune_amplifier_xe/include

# can enable/disable pause with
##include <ittnotify.h>
##ifdef VTUNE
#      __itt_resume();
##endif
##ifdef VTUNE
#      __itt_pause();
##endif

VTUNE_CMD=/sampa/share/intel/vtune_amplifier_xe/bin64/amplxe-cl -collect drepper0 -follow-child -mrte-mode=auto -target-duration-type=short -no-allow-multiple-runs -no-analyze-system -data-limit=100 -slow-frames-threshold=40 -fast-frames-threshold=100

vtune_mpi_run: $(TARGET) $($(MPITYPE)_ENVVAR_TEMP) $($(MPITYPE)_EPILOG_TEMP) $($(MPITYPE)_BATCH_TEMP)
	$(ENV_VARIABLES) \
	$($(MPITYPE)_RUN) $(VTUNE_CMD) -start-paused -- ./$< $($(MPITYPE)_UDPTASKS) $(GARGS)


vtune_run: $(TARGET)
	$(ENV_VARIABLES) \
	$(VTUNE_CMD) -start-paused -- ./$< $(ARGS)

vtune_mpi_test: $(TARGET) $($(MPITYPE)_ENVVAR_TEMP) $($(MPITYPE)_EPILOG_TEMP) $($(MPITYPE)_BATCH_TEMP)
	$(ENV_VARIABLES) \
	$($(MPITYPE)_RUN) $(VTUNE_CMD) -start-paused -- ./$< $($(MPITYPE)_UDPTASKS) --log_level=$(TEST_LOG_LEVEL) --report_level=$(TEST_REPORT_LEVEL) --run_test=$(TARGET:.test=) $(GARGS)

vtune_test: $(TARGET)
	-killall $<
	$(ENV_VARIABLES) \
	 $(VTUNE_CMD) -start-paused -- ./$< --log_level=$(TEST_LOG_LEVEL) --report_level=$(TEST_REPORT_LEVEL) --run_test=$(TARGET:.test=) -- $(ARGS)
