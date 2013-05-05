###
### generic GNU make Makefile for .tex -> .pdf.
### ransford at cs.umass.edu
###   http://github.com/ransford/pdflatex-makefile
###
### Recommended usage:
###   1. echo 'include Makefile.include' > Makefile
###   2. Optional: Edit the Makefile to override $(TARGET)
###      and anything else (e.g., PDFVIEWER, AFTERALL)
###
### Final result:
###   % cat Makefile
###   TARGET=mypaper
###   PDFVIEWER=open -a 'Adobe Acrobat Professional'
###   AFTERALL=mypostprocessingstep
###   include Makefile.include
###
###   mypostprocessingstep:
###           # do something...
###

PDFLATEX	?= pdflatex
BIBTEX		?= bibtex

## Action for 'make view'
OS=$(shell uname -s)
ifeq ($(OS),Darwin)
PDFVIEWER	?= open
else
PDFVIEWER	?= xdg-open
endif

## Name of the target file, minus .pdf: e.g., TARGET=mypaper causes this
## Makefile to turn mypaper.tex into mypaper.pdf.
TARGET ?= report
PDFTARGETS += $(TARGET).pdf

## If $(TARGET).tex refers to .bib files like \bibliography{foo,bar}, then
## $(BIBFILES) will contain foo.bib and bar.bib, and both files will be added as
## dependencies to $(PDFTARGETS).
## Effect: updating a .bib file will trigger re-typesetting.
BIBFILES = $(patsubst %,%.bib,\
		$(shell grep '^[^%]*\\bibliography{' $(TARGET).tex | \
			sed -e 's/^[^%]*\\bibliography{\([^}]*\)}.*/\1/' \
			    -e 's/, */ /g'))

## Add \input'ed or \include'd files to $(PDFTARGETS) dependencies.
INCLUDEDTEX = $(patsubst %,%.tex,\
		$(shell grep '^[^%]*\\\(input\|include\){' $(TARGET).tex | \
			sed 's/[^{]*{\([^}]*\)}.*/\1/'))

AUXFILES = $(foreach T,$(PDFTARGETS:.pdf=), $(T).aux)
AUXFILES += $(patsubst %.tex,%.aux, $(INCLUDEDTEX))
LOGFILES = $(patsubst %.aux,%.log, $(AUXFILES))

## grab a version number from the repository (if any) that stores this.
## * REVISION is the current revision number (short form, for inclusion in text)
## * VCSTURD is a file that gets touched after a repo update
SPACE = $(empty) $(empty)
ifeq ($(shell git status >/dev/null 2>&1 && echo USING_GIT),USING_GIT)
  ifeq ($(shell git svn info >/dev/null 2>&1 && echo USING_GIT_SVN),USING_GIT_SVN)
    # git-svn
    REVISION := $(shell git svn find-rev git-svn)
    VCSTURD := $(subst $(SPACE),\ ,$(shell git rev-parse --show-toplevel)/.git/refs/remotes/git-svn)
  else
    # plain git
    REVISION := $(shell git rev-parse --short HEAD)
    GIT_BRANCH := $(shell git symbolic-ref HEAD 2>/dev/null)
    VCSTURD := $(subst $(SPACE),\ ,$(shell git rev-parse --show-toplevel)/.git/$(GIT_BRANCH))
  endif
else ifeq ($(shell hg root >/dev/null 2>&1 && echo USING_HG),USING_HG)
  # mercurial
  REVISION := $(shell hg id -i)
  VCSTURD := $(subst $(SPACE),\ ,$(shell hg root)/.hg/dirstate)
else ifneq ($(wildcard .svn/entries),)
  # subversion
  REVISION := $(shell svnversion -n)
  VCSTURD := .svn/entries
endif

# .PHONY names all targets that aren't filenames
.PHONY: all clean pdf view

all: pdf $(AFTERALL)

pdf: $(PDFTARGETS)

view: $(PDFTARGETS)
	$(PDFVIEWER) $(PDFTARGETS)

# define a \Revision{} command you can include in your document's preamble.
# especially useful with e.g. drafthead.sty or fancyhdr.
# usage: \input{revision}
#        ... \Revision{}
ifneq ($(REVISION),)
REVDEPS += revision.tex
revision.tex: $(VCSTURD)
  ifeq ($(OS),Darwin)
	echo "\\\\newcommand{\\\\Revision}{$(REVISION)}" > $@
  else
	echo "\\\newcommand{\\Revision}{$(REVISION)}" > $@
  endif
endif

# to generate aux but not pdf from pdflatex, use -draftmode
.INTERMEDIATE: $(AUXFILES)
%.aux: %.tex $(REVDEPS)
	$(PDFLATEX) -draftmode $*

# introduce BibTeX dependency if we found a \bibliography
ifneq ($(strip $(BIBFILES)),)
BIBDEPS = %.bbl
%.bbl: %.aux $(BIBFILES)
	$(BIBTEX) $*
endif

$(PDFTARGETS): %.pdf: %.tex %.aux $(BIBDEPS) $(INCLUDEDTEX) $(REVDEPS)
	$(PDFLATEX) $*
ifneq ($(strip $(BIBFILES)),)
	@if grep -q "undefined references" $*.log; then \
		$(BIBTEX) $* && $(PDFLATEX) $*; fi
endif
	@while grep -q "Rerun to" $*.log; do \
		$(PDFLATEX) $*; done

clean:
	$(RM) $(foreach T,$(PDFTARGETS:.pdf=), \
		$(T).out $(T).pdf $(T).blg $(T).bbl \
		$(T).lof $(T).lot $(T).toc $(T).idx \
		$(T).nav $(T).snm) \
		$(REVDEPS) $(AUXFILES) $(LOGFILES)
