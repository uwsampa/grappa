# Example standalone Grappa app using Grappa's GNU Make include file
#
# To use, build and install Grappa. Then source <Grappa installation
# path>/bin/settings.sh. After that you should be able to just say
# "make" in this directory, and run the generated binary like you do
# any other MPI program.

GRAPPA_IMPLICIT_RULES:=on
include $(GRAPPA_PREFIX)/share/Grappa/grappa.mk

standalone: standalone.o
