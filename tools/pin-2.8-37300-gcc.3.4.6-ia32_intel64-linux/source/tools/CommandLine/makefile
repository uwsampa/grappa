##############################################################
#
# Here are some things you might want to configure
#
##############################################################

TARGET_COMPILER?=gnu
ifdef OS
    ifeq (${OS},Windows_NT)
        TARGET_COMPILER=ms
    endif
endif

##############################################################
#
# include *.config files
#
##############################################################

ifeq ($(TARGET_COMPILER),gnu)
    include ../makefile.gnu.config
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
    DBG?=
endif

TEST_ROOTS = invalid_option invalid_option_logfile noapp

ifeq ($(TARGET_OS),w)
	TEST_ROOTS += invalid_app
endif

TESTS_TO_RUN = $(TEST_ROOTS:%=%.test)

all: 

test: $(TESTS_TO_RUN)


invalid_option.test: invalid_option.tested invalid_option.failed
	if $(PIN) -invalid_option -- $(TESTAPP) makefile makefile.copy.invalid_option >& invalid_option.out; then \
	    echo "Test should return error"; \
		exit 1; \
	fi
	grep "pin error" pin.log*
	rm -f invalid_option.out pin.log*
	rm -f invalid_option.failed makefile.copy.invalid_option

invalid_option_logfile.test: invalid_option_logfile.tested invalid_option_logfile.failed
	if $(PIN) -invalid_option -logfile invalid_option.log -- $(TESTAPP) makefile makefile.copy.invalid_option_logfile >& invalid_option_logfile.out; then \
	    echo "Test should return error"; \
		exit 1; \
	fi
	grep "pin error" invalid_option.log*
	rm -f invalid_option_logfile.out invalid_option.log*
	rm -f invalid_option_logfile.failed makefile.copy.invalid_option_logfile

invalid_app.test: invalid_app.tested invalid_app.failed
	if $(PIN) -- invalid_app >& invalid_app.out; then \
	    echo "Test should return error"; \
		exit 1; \
	fi
	grep "pin error" pin.log*
	rm -f invalid_app.out pin.log*
	rm -f invalid_app.failed

noapp.test: noapp.tested noapp.failed
	if $(PIN) -- >& noapp.out; then \
	    echo "Test should return error"; \
		exit 1; \
	fi
	grep "pin error" pin.log*
	rm -f noapp.out pin.log*
	rm -f noapp.failed


## cleaning
clean:
	-rm -rf *.log *.log* *.out *.tested *.failed 


